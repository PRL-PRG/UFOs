use std::io::Error;
use std::result::Result;
use std::sync::{Arc, Mutex};
use std::{alloc, ffi::c_void};
use std::{
    borrow::BorrowMut,
    collections::{HashMap, VecDeque},
    sync::MutexGuard,
};

use anyhow;

use crossbeam::channel::{Receiver, Sender};
use crossbeam::sync::WaitGroup;
use segment_map::{Segment, SegmentMap};
use userfaultfd::{ReadWrite, Uffd};

use crate::math::*;
use crate::ufo_objects::UfoHandle;

use super::mmap_wrapers::*;
use super::ufo_objects::*;

pub(crate) enum UfoInstanceMsg {
    Shutdown,
    Allocate(promissory::Fulfiller<UfoHandle>, UfoObjectConfig),
    Reset(WaitGroup, UfoId),
    Free(WaitGroup, UfoId),
}

struct UfoWriteBuffer {
    ptr: *mut u8,
    size: usize,
}

/// To avoid allocating memory over and over again for population functions keep around an
/// allocated block and just grow it to the needed capacity
impl UfoWriteBuffer {
    fn new() -> UfoWriteBuffer {
        UfoWriteBuffer {
            size: 0,
            ptr: std::ptr::null_mut(),
        }
    }

    unsafe fn ensure_capcity(&mut self, capacity: usize) -> *mut u8 {
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
        self.ptr
    }

    unsafe fn slice(&self) -> &[u8] {
        std::slice::from_raw_parts(self.ptr, self.size)
    }
    // unsafe fn slice_mut(&self) -> &mut [u8] {
    //     std::slice::from_raw_parts_mut(self.ptr, self.size)
    // }
}

struct UfoChunks {
    loaded_chunks: VecDeque<UfoChunk>,
    used_memory: usize,
    config: Arc<UfoCoreConfig>,
}

impl UfoChunks {
    fn new(config: Arc<UfoCoreConfig>) -> UfoChunks {
        UfoChunks {
            loaded_chunks: VecDeque::new(),
            used_memory: 0,
            config,
        }
    }

    fn add(&mut self, chunk: UfoChunk) {
        self.used_memory = self.used_memory + chunk.size();
        self.loaded_chunks.push_back(chunk);
    }

    fn drop_ufo_chunks(&mut self, ufo_id: UfoId) {
        let chunks = &mut self.loaded_chunks;
        chunks
            .iter_mut()
            .filter(|c| c.ufo_id() == ufo_id)
            .for_each(UfoChunk::mark_freed);
        self.used_memory = chunks.iter().map(UfoChunk::size).sum();
    }

    fn free_until_low_water_mark(&mut self) -> anyhow::Result<usize> {
        let low_water_mark = self.config.low_watermark;
        while self.used_memory > low_water_mark {
            match self.loaded_chunks.pop_front().borrow_mut() {
                None => anyhow::bail!("nothing to free"),
                Some(chunk) => {
                    let size = chunk.free_and_writeback_dirty()?;
                    self.used_memory -= size;
                }
            }
        }
        Ok(self.used_memory)
    }
}

pub struct UfoCoreConfig {
    pub writeback_temp_path: Box<str>,
    high_watermark: usize,
    low_watermark: usize,
}

pub(crate) type WrappedUfoObject = Arc<Mutex<UfoObject>>;

pub(crate) struct UfoCoreState {
    object_id_gen: UfoIdGen,

    objects_by_id: HashMap<UfoId, WrappedUfoObject>,
    objects_by_segment: SegmentMap<usize, WrappedUfoObject>,

    loaded_chunks: UfoChunks,
}

pub(crate) struct UfoCore {
    uffd: Uffd,
    pub config: Arc<UfoCoreConfig>,

    pub msg_send: Sender<UfoInstanceMsg>,
    msg_recv: Receiver<UfoInstanceMsg>,

    state: Mutex<UfoCoreState>,
}

impl UfoCore {
    pub(crate) fn new(config: UfoCoreConfig) -> Result<Arc<UfoCore>, Error> {
        // If this fails then there is nothing we should even try to do about it honestly
        let uffd = userfaultfd::UffdBuilder::new()
            .close_on_exec(true)
            .non_blocking(false)
            .create()
            .unwrap();

        let config = Arc::new(config);
        let (send, recv) = crossbeam::channel::bounded(2);

        let state = Mutex::new(UfoCoreState {
            object_id_gen: UfoIdGen::new(),

            loaded_chunks: UfoChunks::new(Arc::clone(&config)),
            objects_by_id: HashMap::new(),
            objects_by_segment: SegmentMap::new(),
        });

        let core = Arc::new(UfoCore {
            uffd,
            config,
            msg_send: send,
            msg_recv: recv,
            state,
        });

        let pop_core = Arc::clone(&core);
        std::thread::spawn(move || UfoCore::populate_loop(pop_core));

        let msg_core = Arc::clone(&core);
        std::thread::spawn(move || UfoCore::msg_loop(msg_core));

        // TODO: seems like we should pass back something wrapping the Arc that can shutdown the core?
        Ok(core)
    }

    fn get_locked_state(&self) -> anyhow::Result<MutexGuard<UfoCoreState>> {
        match self.state.lock() {
            Err(_) => return Err(anyhow::Error::msg("broken lock")),
            Ok(l) => Ok(l),
        }
    }

    fn ensure_capcity(config: &UfoCoreConfig, state: &mut UfoCoreState, to_load: usize) {
        assert!(to_load + config.low_watermark < config.high_watermark);
        if to_load + state.loaded_chunks.used_memory > config.high_watermark {
            state.loaded_chunks.free_until_low_water_mark().unwrap();
        }
    }

    //TODO: this loop won't shutdown as it stands, it needs to be given a signal or such that it should exit
    fn populate_loop(this: Arc<UfoCore>) {
        fn populate_impl(core: &UfoCore, buffer: &mut UfoWriteBuffer, addr: *mut c_void) {
            let state = &mut *core.get_locked_state().unwrap();

            let ptr_int = addr as usize;

            // blindly unwrap here because if we get a message for an address we don't have then it is explodey time
            // clone the arc so we aren't borrowing the state
            let ufo_arc = state.objects_by_segment.get(&ptr_int).unwrap().clone();
            let ufo = ufo_arc.lock().unwrap();

            let config = &ufo.config;

            let load_size = config.elements_loaded_at_once * config.stride;

            let base_int = ufo.mmap.as_ptr() as usize;
            assert!(ptr_int > base_int);
            let base_after_header = base_int + config.header_size_with_padding;
            assert!(ptr_int > base_after_header);
            let fault_offset = ptr_int - base_after_header;
            let fault_segment_start = down_to_nearest(fault_offset, load_size);

            let start = fault_segment_start / config.stride;
            let end = start + config.elements_loaded_at_once;

            // Before we perform the load ensure that there is capacity
            UfoCore::ensure_capcity(&core.config, state, load_size);

            unsafe {
                let buffer_ptr = buffer.ensure_capcity(load_size);
                (config.populate)(start, end, buffer.ptr);
                core.uffd
                    .copy(buffer_ptr.cast(), addr, load_size, true)
                    .expect("unable to populate range");
            }

            let raw_data = unsafe { &buffer.slice()[0..load_size] };
            assert!(raw_data.len() == load_size);
            let chunk = UfoChunk::new(&ufo_arc, fault_segment_start, raw_data);
            state.loaded_chunks.add(chunk);
        }

        let uffd = &this.uffd;
        let mut buffer = UfoWriteBuffer::new();

        loop {
            match uffd.read_event() {
                Ok(Some(event)) => match event {
                    userfaultfd::Event::Pagefault { rw, addr } => match rw {
                        ReadWrite::Read => populate_impl(&*this, &mut buffer, addr),
                        ReadWrite::Write => panic!("unexpected write fault"),
                    },
                    e => panic!("Recieved an event we did not register for {:?}", e),
                },
                Ok(None) => { /*huh*/ }
                Err(userfaultfd::Error::ReadEof) => return (/*done*/),
                err => {
                    err.expect("uffd read error");
                }
            }
        }
    }

    fn msg_loop(this: Arc<UfoCore>) {
        fn allocate_impl(
            this: &Arc<UfoCore>,
            config: UfoObjectConfig,
        ) -> anyhow::Result<UfoHandle> {
            let state = &mut *this.get_locked_state()?;

            let id_map = &state.objects_by_id;
            let id_gen = &mut state.object_id_gen;

            let id = id_gen.next(|k| id_map.contains_key(k));

            let mmap = BaseMmap::new(
                config.true_size,
                &[MemoryProtectionFlag::Read, MemoryProtectionFlag::Write],
                &[MmapFlag::Annon, MmapFlag::Private],
                None,
            )
            .expect("Mmap Error");

            let mmap_ptr = mmap.as_ptr();
            let true_size = config.true_size;
            let mmap_base = mmap_ptr as usize;
            let segment = Segment::new(mmap_base, mmap_base + true_size);

            let writeback = UfoFileWriteback::new(&config, this)?;
            this.uffd.register(mmap_ptr.cast(), true_size)?;

            let c_ptr = mmap.as_ptr().cast();
            let ufo = UfoObject {
                id,
                config,
                mmap,
                writeback_util: writeback,
            };

            let ufo = Arc::new(Mutex::new(ufo));

            state.objects_by_id.insert(id, ufo.clone());
            state.objects_by_segment.insert(segment, ufo.clone());

            Ok(UfoHandle { id, ptr: c_ptr })
        }

        fn reset_impl(this: &Arc<UfoCore>, ufo_id: UfoId) -> anyhow::Result<()> {
            let state = &mut *this.get_locked_state()?;

            let ufo = &mut *(state
                .objects_by_id
                .get(&ufo_id)
                .map(Ok)
                .unwrap_or_else(|| Err(anyhow::anyhow!("unknown ufo")))?
                .lock()
                .map_err(|_| anyhow::anyhow!("lock poisoned"))?);

            ufo.reset()?;

            state.loaded_chunks.drop_ufo_chunks(ufo_id);

            Ok(())
        }

        fn free_impl(this: &Arc<UfoCore>, ufo_id: UfoId) -> anyhow::Result<()> {
            let state = &mut *this.get_locked_state()?;
            let ufo = state
                .objects_by_id
                .remove(&ufo_id)
                .map(Ok)
                .unwrap_or_else(|| Err(anyhow::anyhow!("No such Ufo")))?;
            let ufo = ufo.lock().map_err(|_| anyhow::anyhow!("Broken Ufo Lock"))?;

            let mmap_base = ufo.mmap.as_ptr() as usize;
            let segment = state
                .objects_by_segment
                .get_entry(&mmap_base)
                .map(Ok)
                .unwrap_or_else(|| Err(anyhow::anyhow!("memory segment missing")))?
                .0
                .clone();

            state.objects_by_segment.remove(&segment);

            state.loaded_chunks.drop_ufo_chunks(ufo_id);

            Ok(())
        }

        let recv = &this.msg_recv;
        loop {
            match recv.recv() {
                Ok(m) => match m {
                    UfoInstanceMsg::Allocate(fulfiller, cfg) => {
                        fulfiller.fulfill(allocate_impl(&this, cfg).expect("Allocate Error"))
                    }
                    UfoInstanceMsg::Reset(_, ufo_id) => {
                        reset_impl(&this, ufo_id).expect("Reset Error")
                    }
                    UfoInstanceMsg::Free(_, ufo_id) => {
                        free_impl(&this, ufo_id).expect("Free Error")
                    }
                    UfoInstanceMsg::Shutdown => return (/*done*/),
                },
                err => {
                    err.expect("recv error");
                }
            }
        }
    }

    pub(crate) fn shutdown(&self) {
        self.msg_send
            .send(UfoInstanceMsg::Shutdown)
            .expect("Can't send shutdown signal");

        let fd = std::os::unix::prelude::AsRawFd::as_raw_fd(&self.uffd);
        // this will signal to the populate loop that it is time to close down
        unsafe { libc::close(fd) };
    }
}
