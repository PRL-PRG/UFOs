use std::io::Error;
use std::num::NonZeroUsize;
use std::sync::{Arc, Mutex, Weak};
use std::{lazy::SyncLazy, sync::MutexGuard};

use anyhow::Result;
use crossbeam::sync::WaitGroup;
use thiserror::Error;

use log::{debug, error, trace};

use num::Integer;

use crate::mmap_wrapers;

use super::errors::*;
use super::math::*;
use super::mmap_wrapers::*;
use super::return_checks::*;
use super::ufo_core::*;

pub static PAGE_SIZE: SyncLazy<usize> = SyncLazy::new(|| {
    let page_size = unsafe { libc::sysconf(libc::_SC_PAGE_SIZE) };
    assert!(page_size > 0);
    page_size as usize
});

#[derive(Debug, PartialEq, PartialOrd, Ord, Eq, Copy, Clone, Hash)]
#[repr(C)]
pub struct UfoId(pub(crate) u64);

impl UfoId {
    pub fn sentinel() -> Self {
        UfoId(0)
    }

    pub fn is_sentinel(&self) -> bool {
        0 == self.0
    }
}

pub struct UfoIdGen {
    current: u64,
}

type DataHash = blake3::Hash;

fn hash_function(data: &[u8]) -> DataHash {
    if data.len() > 128 * 1024 {
        // On large blocks we can get significant gains from parallelism
        blake3::Hasher::new()
            .update_with_join::<blake3::join::RayonJoin>(data)
            .finalize()
    } else {
        blake3::hash(data)
    }
}

impl UfoIdGen {
    pub fn new() -> UfoIdGen {
        UfoIdGen { current: 0 }
    }

    pub(crate) fn next<P>(&mut self, is_unused: P) -> UfoId
    where
        P: Fn(&UfoId) -> bool,
    {
        let mut n = self.current;
        let mut id;
        loop {
            n = n.wrapping_add(1);
            id = UfoId(n);
            if is_unused(&id) {
                break;
            }
        }
        self.current = n;
        id
    }
}

pub struct UfoObjectConfigPrototype {
    pub header_size: usize,
    pub stride: usize,
    pub min_load_ct: Option<usize>,
}

impl UfoObjectConfigPrototype {
    pub fn new_prototype(
        header_size: usize,
        stride: usize,
        min_load_ct: Option<usize>,
    ) -> UfoObjectConfigPrototype {
        UfoObjectConfigPrototype {
            header_size,
            stride,
            min_load_ct,
        }
    }

    pub fn new_config(&self, ct: usize, populate: Box<UfoPopulateFn>) -> UfoObjectConfig {
        UfoObjectConfig::new_config(
            self.header_size,
            ct,
            self.stride,
            self.min_load_ct,
            populate,
        )
    }
}

pub struct UfoObjectConfig {
    pub(crate) populate: Box<UfoPopulateFn>,

    pub(crate) header_size_with_padding: usize,
    pub(crate) header_size: usize,

    pub(crate) stride: usize,
    pub(crate) elements_loaded_at_once: usize,
    pub(crate) element_ct: usize,
    pub(crate) true_size: usize,
}

impl UfoObjectConfig {
    pub(crate) fn new_config(
        header_size: usize,
        element_ct: usize,
        stride: usize,
        min_load_ct: Option<usize>,
        populate: Box<UfoPopulateFn>,
    ) -> UfoObjectConfig {
        let min_load_ct = min_load_ct.unwrap_or(1);
        let page_size = mmap_wrapers::get_page_size();

        /* Headers and size */
        let header_size_with_padding = up_to_nearest(header_size as usize, page_size);
        let body_size_with_padding = up_to_nearest(stride * element_ct, page_size);
        let true_size = header_size_with_padding + body_size_with_padding;

        /* loading quanta */
        let min_load_bytes = num::integer::lcm(page_size, stride * min_load_ct);
        let elements_loaded_at_once = min_load_bytes / stride;
        assert!(elements_loaded_at_once * stride == min_load_bytes);

        UfoObjectConfig {
            header_size,
            stride,

            header_size_with_padding,
            true_size,

            elements_loaded_at_once,
            element_ct,

            populate,
        }
    }
}

pub(crate) struct UfoOffset {
    base_addr: usize,
    stride: usize,
    header_bytes: usize,
    absolute_offset_bytes: usize,
}

impl UfoOffset {
    pub fn from_addr(ufo: &UfoObject, addr: *const libc::c_void) -> UfoOffset {
        let addr = addr as usize;
        let base_addr = ufo.mmap.as_ptr() as usize;
        let absolute_offset_bytes = addr
            .checked_sub(base_addr)
            .unwrap_or_else(|| panic!("Addr less than base {} < {}", addr, base_addr));
        let header_bytes = ufo.config.header_size_with_padding;

        assert!(
            header_bytes <= absolute_offset_bytes,
            "Cannot offset into the header"
        );

        UfoOffset {
            base_addr,
            stride: ufo.config.stride,
            header_bytes,
            absolute_offset_bytes,
        }
    }

    pub fn absolute_offset(&self) -> usize {
        self.absolute_offset_bytes
    }

    pub fn offset_from_header(&self) -> usize {
        self.absolute_offset_bytes - self.header_bytes
    }

    pub fn as_ptr_int(&self) -> usize {
        self.base_addr + self.absolute_offset()
    }

    pub fn as_index_floor(&self) -> usize {
        self.offset_from_header().div_floor(&self.stride)
    }

    pub fn down_to_nearest_n_relative_to_header(&self, nearest: usize) -> UfoOffset {
        let offset = self.offset_from_header();
        let offset = down_to_nearest(offset, nearest);

        let absolute_offset_bytes = self.header_bytes + offset;

        UfoOffset {
            absolute_offset_bytes,
            ..*self
        }
    }
}

pub(crate) struct UfoChunk {
    ufo_id: UfoId,
    object: Weak<Mutex<UfoObject>>,
    offset: UfoOffset,
    length: Option<NonZeroUsize>,
    hash: DataHash,
}

impl UfoChunk {
    pub fn new(
        arc: &WrappedUfoObject,
        object: &MutexGuard<UfoObject>,
        offset: UfoOffset,
        initial_data: &[u8],
    ) -> UfoChunk {
        assert!(offset.absolute_offset()  + initial_data.len() <= object.mmap.length(),
        "{} + {} > {}", offset.absolute_offset(), initial_data.len(), object.mmap.length());
        UfoChunk {
            ufo_id: object.id,
            object: Arc::downgrade(arc),
            offset,
            length: NonZeroUsize::new(initial_data.len()),
            hash: hash_function(initial_data),
        }
    }

    fn with_slice<F, V>(&self, obj: &UfoObject, f: F) -> Option<V>
    where
        F: FnOnce(&[u8]) -> V,
    {
        self.length.and_then(|length| {
            obj.mmap
                .with_slice(self.offset.absolute_offset(), length.get(), f)
        })
    }

    pub fn free_and_writeback_dirty(&mut self) -> Result<usize> {
        if let Some(length) = self.length {
            let length = length.get();
            if let Some(obj) = self.object.upgrade() {
                let mut obj = obj.lock().unwrap();

                trace!(target: "ufo_object", "free chunk {:?}@{} ({}b)",
                    self.ufo_id, self.offset.absolute_offset() , length
                );

                let calculated_hash = obj
                    .mmap
                    .with_slice(self.offset.absolute_offset(), length, hash_function)
                    .unwrap(); // it should never be possible for this to fail
                trace!(target: "ufo_object", "writeback hash matches {}", self.hash == calculated_hash);
                if self.hash != calculated_hash {
                    let o = &mut *obj;
                    o.writeback(self)?;
                }

                unsafe {
                    let ptr = obj.mmap.as_ptr().add(self.offset.absolute_offset());
                    // MADV_DONTNEED has the exact semantics we want, no other advice would work for us
                    check_return_zero(libc::madvise(ptr.cast(), length, libc::MADV_DONTNEED))?;
                }
            }
            self.length = None;
            Ok(length)
        } else {
            Ok(0)
        }
    }

    pub fn mark_freed(&mut self) {
        self.length = None;
    }

    pub fn ufo_id(&self) -> UfoId {
        self.ufo_id
    }

    pub fn size(&self) -> usize {
        self.length.map(NonZeroUsize::get).unwrap_or(0)
    }
}

#[derive(Error, Debug)]
#[error("Internal Ufo Error when populating")]
pub struct UfoPopulateError;

pub type UfoPopulateFn =
    dyn Fn(usize, usize, *mut u8) -> Result<(), UfoPopulateError> + Sync + Send;
pub(crate) struct UfoFileWriteback {
    ufo_id: UfoId,
    mmap: MmapFd,
    chunk_size: usize,
    total_bytes: usize,
    bitmap_bytes: usize,
}

impl UfoFileWriteback {
    pub fn new(
        ufo_id: UfoId,
        cfg: &UfoObjectConfig,
        core: &Arc<UfoCore>,
    ) -> Result<UfoFileWriteback, Error> {
        let page_size = *PAGE_SIZE;

        let chunk_ct = cfg.element_ct.div_ceil(&cfg.elements_loaded_at_once);
        assert!(chunk_ct * cfg.elements_loaded_at_once >= cfg.element_ct);

        let chunk_size = cfg.elements_loaded_at_once * cfg.stride;

        let bitmap_bytes = chunk_ct.div_ceil(&8); /*8 bits per byte*/
        // Now we want to get the bitmap bytes up to the next multiple of the page size
        let bitmap_bytes = up_to_nearest(bitmap_bytes, page_size);
        assert!(bitmap_bytes * 8 >= chunk_ct);
        assert!(bitmap_bytes.trailing_zeros() >= page_size.trailing_zeros());

        let data_bytes = cfg.element_ct * cfg.stride;
        let total_bytes = bitmap_bytes + data_bytes;

        let temp_file =
            unsafe { OpenFile::temp(core.config.writeback_temp_path.as_str(), total_bytes) }?;

        let mmap = MmapFd::new(
            total_bytes,
            &[MemoryProtectionFlag::Read, MemoryProtectionFlag::Write],
            &[MmapFlag::Shared],
            None,
            temp_file,
            0,
        )?;

        Ok(UfoFileWriteback {
            ufo_id,
            chunk_size,
            mmap,
            total_bytes,
            bitmap_bytes,
        })
    }

    fn body_bytes(&self) -> usize {
        self.total_bytes - self.bitmap_bytes
    }

    pub fn writeback_function<'a>(
        &'a self,
        offset: &UfoOffset,
    ) -> Result<Box<dyn FnOnce(&[u8]) + 'a>> {
        let off_head = offset.offset_from_header();
        if off_head > self.body_bytes() {
            anyhow::bail!("{} outside of range", off_head);
        }

        let chunk_number = off_head.div_floor(&self.chunk_size);
        let writeback_offset = self.bitmap_bytes + off_head;

        let chunk_byte = chunk_number >> 3;
        let chunk_bit = 1u8 << (chunk_number & 0b111);

        debug!(target: "ufo_object", "writeback offset {:#x}", writeback_offset);

        Ok(Box::new(move |live_data| {
            let bitmap_ptr: &mut u8 =
                unsafe { self.mmap.as_ptr().add(chunk_byte).as_mut().unwrap() };
            let expected_size = std::cmp::min(
                self.chunk_size,
                self.total_bytes - writeback_offset);

            let writeback_arr: &mut [u8] = unsafe {
                std::slice::from_raw_parts_mut(
                    self.mmap.as_ptr().add(writeback_offset),
                    expected_size,
                )
            };

            assert!(live_data.len() == expected_size);

            *bitmap_ptr |= chunk_bit;
            writeback_arr.copy_from_slice(live_data);
            trace!(target: "ufo_object", "writeback");
        }))
    }

    pub fn try_readback<'a>(&'a self, offset: &UfoOffset) -> Option<&'a [u8]> {
        let off_head = offset.offset_from_header();
        trace!(target: "ufo_object", "try readback {:?}@{:#x}", self.ufo_id, off_head);

        let chunk_number = off_head.div_floor(&self.chunk_size);
        let readback_offset = self.bitmap_bytes + off_head;

        let chunk_byte = chunk_number >> 3;
        let chunk_bit = 1u8 << (chunk_number & 0b111);

        let bitmap_ptr: &u8 = unsafe { self.mmap.as_ptr().add(chunk_byte).as_ref().unwrap() };
        let is_written = *bitmap_ptr & chunk_bit != 0;

        if is_written {
            trace!(target: "ufo_object", "allow readback {:?}@{:#x}", self.ufo_id, off_head);
            let arr: &[u8] = unsafe {
                std::slice::from_raw_parts(self.mmap.as_ptr().add(readback_offset), self.chunk_size)
            };
            Some(arr)
        } else {
            None
        }
    }

    pub fn reset(&self) -> Result<()> {
        let ptr = self.mmap.as_ptr();
        unsafe {
            check_return_zero(libc::madvise(
                ptr.cast(),
                self.total_bytes,
                // punch a hole in the file
                libc::MADV_REMOVE,
            ))?;
        }
        Ok(())
    }
}

pub struct UfoObject {
    pub id: UfoId,
    pub core: Weak<UfoCore>,
    pub config: UfoObjectConfig,
    pub mmap: BaseMmap,
    pub(crate) writeback_util: UfoFileWriteback,
}

impl UfoObject {
    fn writeback(&mut self, chunk: &UfoChunk) -> Result<()> {
        //TODO: Flag that we wrote out this chunk
        //TODO: Test for written out chunks
        chunk
            .with_slice(self, |live_data| {
                debug!(target: "ufo_object", "writeback {:?}@{:#x}:{}",
                    self.id,
                    self.mmap.as_ptr() as usize + chunk.offset.absolute_offset(),
                    live_data.len(),
                );

                self.writeback_util
                    .writeback_function(&chunk.offset)
                    .unwrap()(live_data);
            })
            .map(Ok)
            .unwrap_or_else(|| {
                Err(Error::new(std::io::ErrorKind::AddrNotAvailable, "Chunk not valid").into())
            })
    }

    pub(crate) fn reset_internal(&self) -> anyhow::Result<()> {
        let length = self.config.true_size - self.config.header_size_with_padding;
        unsafe {
            check_return_zero(libc::madvise(
                self.mmap.as_ptr().add(self.config.header_size_with_padding).cast(),
                length,
                libc::MADV_DONTNEED,
            ))?;
        }
        self.writeback_util.reset()?;

        Ok(())
    }

    pub fn header_ptr(&self) -> *mut std::ffi::c_void {
        let header_offset = self.config.header_size_with_padding - self.config.header_size;
        unsafe { self.mmap.as_ptr().add(header_offset).cast() }
    }

    pub fn body_ptr(&self) -> *mut std::ffi::c_void {
        unsafe {
            self.mmap
                .as_ptr()
                .add(self.config.header_size_with_padding)
                .cast()
        }
    }

    pub fn reset(&self) -> Result<WaitGroup, UfoLookupErr> {
        let wait_group = crossbeam::sync::WaitGroup::new();
        let core = match self.core.upgrade() {
            None => return Err(UfoLookupErr::CoreShutdown),
            Some(x) => x,
        };

        core.msg_send
            .send(UfoInstanceMsg::Reset(wait_group.clone(), self.id))?;

        Ok(wait_group)
    }

    pub fn free(&self) -> Result<WaitGroup, UfoLookupErr> {
        let wait_group = crossbeam::sync::WaitGroup::new();
        let core = match self.core.upgrade() {
            None => return Err(UfoLookupErr::CoreShutdown),
            Some(x) => x,
        };

        core.msg_send
            .send(UfoInstanceMsg::Free(wait_group.clone(), self.id))?;

        Ok(wait_group)
    }
}
