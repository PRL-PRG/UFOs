use segment_map::SegmentMap;
use std::alloc;
use std::cell::RefCell;
use std::collections::{HashMap, VecDeque};

use blake3;
use libc;
use std::io::Error;
use std::lazy::SyncLazy;
use std::num::NonZeroUsize;
use std::ops::Deref;
use std::os::unix::io::RawFd;
use std::ptr::NonNull;
use std::rc::{Rc, Weak};
use std::result::Result;
use std::sync::mpsc::{Receiver, Sender};
use std::thread::Thread;
use userfaultfd::*;

use num::One;
use std::ops::{Add, Div, Mul, Sub};

use super::mmap_wrapers::*;

static PAGE_SIZE: SyncLazy<usize> = SyncLazy::new(|| {
    let page_size = unsafe { libc::sysconf(libc::_SC_PAGE_SIZE) };
    assert!(page_size > 0);
    page_size as usize
});

enum UfoInstanceMsg {
    Shutdown,
    Allocate(UfoObjectConfig),
    Reset(UfoId),
    Free(UfoId),
}

struct UfoWriteBuffer {
    ptr: *mut u8,
    size: usize,
}

/// To avoid allocating memory over and over again for population functions keep around an
/// allocated block and just grow it to the needed capacity
impl UfoWriteBuffer {
    unsafe fn ensure_capcity(&mut self, capacity: usize) {
        if self.size < capacity {
            let layout = alloc::Layout::from_size_align(self.size, *PAGE_SIZE).unwrap();
            let new_ptr = alloc::realloc(self.ptr, layout, capacity);

            if new_ptr.is_null() {
                alloc::handle_alloc_error(layout);
            } else {
                self.ptr = new_ptr;
                self.size = capacity;
            }
        }
    }
}

#[derive(Debug, PartialEq, Eq, Copy, Clone, Hash)]
struct UfoId(u64);

struct UfoIdGen {
    current: u64,
}

impl UfoIdGen {
    fn next<P>(&mut self, is_unused: P) -> UfoId
    where
        P: Fn(u64) -> bool,
    {
        let mut n = self.current;
        loop {
            n = n.wrapping_add(1);
            if is_unused(n) {
                break;
            }
        }
        self.current = n;
        UfoId(n)
    }
}

struct UfoChunks {
    loaded_chunks: VecDeque<UfoChunk>,
    high_watermark: usize,
    low_watermark: usize,
    used_memory: usize,
}

pub struct UfoObjectConfig {
    populate: Box<UfoPopulateFn>,

    header_size_with_padding: usize,
    header_size: usize,

    stride: usize,
    elements_loaded_at_once: usize,
    element_ct: usize,
    true_size: usize,
}

pub struct UfoObject {
    id: UfoId,
    config: UfoObjectConfig,
    core: Weak<UfoCore>,
    mmap: BaseMmap,
    writeback: UfoFileWriteback,
}

pub struct UfoChunk {
    ufo_id: UfoId,
    offset: usize,
    length: Option<NonZeroUsize>,
    hash: blake3::Hash,
}

fn check_return_zero(result: i32) -> Result<(), Error> {
    match result {
        0 => Ok(()),
        -1 => Err(Error::last_os_error()),
        _ => panic!("return other than 0 or -1 ({})", result),
    }
}

impl UfoChunk {
    fn new(ufo_id: UfoId, offset: usize, initial_data: &[u8]) -> UfoChunk {
        UfoChunk {
            ufo_id,
            offset,
            length: NonZeroUsize::new(initial_data.len()),
            hash: blake3::hash(initial_data),
        }
    }

    fn with_slice<F, V>(&self, core: &UfoCore, f: F) -> Option<V>
    where
        F: FnOnce(&[u8]) -> V,
    {
        self.length.and_then(|length| {
            core.objects_by_id.get(&self.ufo_id).and_then(|obj| {
                let obj = obj.as_ref().borrow();
                obj.mmap.with_slice(self.offset, length.get(), f)
            })
        })
    }

    fn has_changes(&self, core: &UfoCore) -> Option<bool> {
        self.with_slice(core, |live_data| self.hash == blake3::hash(live_data))
    }

    fn free(&mut self, core: &UfoCore) -> Result<(), Error> {
        if let Some(length) = self.length {
            if let Some(obj) = core.objects_by_id.get(&self.ufo_id) {
                let obj = obj.as_ref().borrow();
                unsafe {
                    let ptr = obj.mmap.as_ptr().offset(self.offset as isize);
                    // MADV_DONTNEED has the exact semantics we want, no other advice would work for us
                    check_return_zero(libc::madvise(
                        ptr.cast(),
                        length.get(),
                        libc::MADV_DONTNEED,
                    ))?;
                }
            }
            self.length = None;
        }
        Ok(())
    }
}

pub type UfoPopulateFn = dyn FnMut(usize, usize, *mut u8);
pub struct UfoFileWriteback {
    mmap: MmapFd,
    core: Weak<UfoCore>,
    total_bytes: usize,
    bitmap_bytes: usize,
}

fn ceiling_div<T>(a: T, b: T) -> T
where
    T: Div<T, Output = T> + Add<T, Output = T> + Sub<T, Output = T> + One + Copy,
{
    let b_m1: T = b - <T as One>::one();
    (a + b_m1) / b
}

fn up_to_nearest<T>(a: T, nearest: T) -> T
where
    T: Div<T, Output = T>
        + Add<T, Output = T>
        + Mul<T, Output = T>
        + Sub<T, Output = T>
        + Copy
        + One,
{
    ceiling_div(a, nearest) * nearest
}

impl UfoFileWriteback {
    pub fn new(object: &UfoObject) -> Result<UfoFileWriteback, Error> {
        let cfg = &object.config;
        let page_size = *PAGE_SIZE;

        // If we got here the core must be active
        let core = object.core.upgrade().unwrap();

        // cieling div
        let chunk_ct = ceiling_div(cfg.element_ct, cfg.elements_loaded_at_once);
        let bitmap_bytes = chunk_ct >> 3; // 8 bits per byte
                                          // Now we want to get the bitmap bytes up to the next multiple of the page size
        let bitmap_bytes = up_to_nearest(bitmap_bytes, page_size);

        let data_bytes = cfg.element_ct * cfg.stride;
        let total_bytes = bitmap_bytes + data_bytes;

        let temp_file = unsafe { OpenFile::temp(core.config.writeback_temp_path.as_ref(), total_bytes) }?;

        let mmap = MmapFd::new(
                total_bytes,
                &[MemoryProtectionFlag::Read, MemoryProtectionFlag::Write],
                &[MmapFlag::Shared],
                None,
                temp_file,
                0,
            )?;

        let core = Rc::downgrade(&core);

        Ok(UfoFileWriteback {
            mmap,
            core,
            total_bytes,
            bitmap_bytes,
        })
    }

    fn writeback(&mut self, chunk: &UfoChunk) -> Result<(), Error> {
        // we blindly unwrap here because if we got called the core must exist
        let core = self.core.upgrade().unwrap();
        let core = core.as_ref();

        chunk
            .with_slice(core, |live_data| {
                self.mmap
                    .with_slice_mut(chunk.offset, live_data.len(), |writeback_arr| {
                        assert!(live_data.len() == writeback_arr.len());
                        writeback_arr.copy_from_slice(live_data)
                    })
            })
            .and_then(|x| x)
            .map(Ok)
            .unwrap_or(Err(Error::new(
                std::io::ErrorKind::AddrNotAvailable,
                "Chunk not valid",
            )))
    }
}

/*
 *  UFO Core impl
 */

pub struct UfoCoreConfig {
    writeback_temp_path: Box<str>,
}

pub struct UfoCore {
    uffd: Uffd,
    config: UfoCoreConfig,

    msgSend: Sender<UfoInstanceMsg>,
    msgRecv: Receiver<UfoInstanceMsg>,

    populate_thread: Thread,
    control_thread: Thread,

    buffer: UfoWriteBuffer,
    object_id_gen: UfoIdGen,

    objects_by_id: HashMap<UfoId, Rc<RefCell<UfoObject>>>,
    objects_by_segment: SegmentMap<UfoId, Rc<RefCell<UfoObject>>>,
}

impl UfoCore {
    fn new() {}
}
