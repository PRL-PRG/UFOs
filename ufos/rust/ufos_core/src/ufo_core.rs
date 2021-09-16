use std::result::Result;
use std::sync::{Arc, Mutex, RwLock};
use std::{alloc, ffi::c_void};
use std::{
    cmp::min,
    ops::{Deref, Range},
    vec::Vec,
};
use std::{
    collections::{HashMap, VecDeque},
    sync::MutexGuard,
};

use log::{debug, info, trace, warn};

use btree_interval_map::IntervalMap;
use crossbeam::channel::{Receiver, Sender};
use crossbeam::sync::WaitGroup;
use rayon::iter::{IntoParallelIterator, ParallelIterator};
use userfaultfd::Uffd;

use crate::once_await::OnceFulfiller;
use crate::populate_workers::{PopulateWorkers, RequestWorker, ShouldRun};

use super::errors::*;
use super::mmap_wrapers::*;
use super::ufo_objects::*;

pub enum UfoInstanceMsg {
    Shutdown(WaitGroup),
    Allocate(promissory::Fulfiller<WrappedUfoObject>, UfoObjectConfig),
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

impl Drop for UfoWriteBuffer {
    fn drop(&mut self) {
        if self.size > 0 {
            let layout = alloc::Layout::from_size_align(self.size, *PAGE_SIZE).unwrap();
            unsafe {
                alloc::dealloc(self.ptr, layout);
            }
        }
    }
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
        self.used_memory += chunk.size();
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
        debug!(target: "ufo_core", "Freeing memory");
        let low_water_mark = self.config.low_watermark;

        let mut to_free = Vec::new();
        let mut will_free_bytes = 0;

        while self.used_memory - will_free_bytes > low_water_mark {
            match self.loaded_chunks.pop_front() {
                None => anyhow::bail!("nothing to free"),
                Some(chunk) => {
                    let size = chunk.size(); // chunk.free_and_writeback_dirty()?;
                    will_free_bytes += size;
                    to_free.push(chunk);
                    // self.used_memory -= size;
                }
            }
        }

        let freed_memory = to_free
            .into_par_iter()
            .map_init(ChunkFreer::new, |f, mut c| f.free_chunk(&mut c))
            .reduce(|| Ok(0), |a, b| Ok(a? + b?))?;

        debug!(target: "ufo_core", "Done freeing memory");

        self.used_memory -= freed_memory;
        assert!(self.used_memory <= low_water_mark);

        Ok(self.used_memory)
    }
}

pub struct UfoCoreConfig {
    pub writeback_temp_path: String,
    pub high_watermark: usize,
    pub low_watermark: usize,
}

pub type WrappedUfoObject = Arc<RwLock<UfoObject>>;

pub struct UfoCoreState {
    object_id_gen: UfoIdGen,

    objects_by_id: HashMap<UfoId, WrappedUfoObject>,
    objects_by_segment: IntervalMap<usize, WrappedUfoObject>,

    loaded_chunks: UfoChunks,
}

pub struct UfoCore {
    uffd: Uffd,
    pub config: Arc<UfoCoreConfig>,

    pub msg_send: Sender<UfoInstanceMsg>,
    state: Mutex<UfoCoreState>,
}

impl UfoCore {
    // pub fn print_segments(&self) {
    //     self.state
    //         .lock()
    //         .expect("core locked")
    //         .objects_by_segment
    //         .iter()
    //         .for_each(|e| {
    //             eprintln!(
    //                 "{:?}: {:#x}-{:#x}",
    //                 e.value.read().expect("locked ufo").id,
    //                 e.start,
    //                 e.end
    //             )
    //         });
    // }

    // pub fn assert_segment_map(&self){
    //     let core = self.state.lock().expect("core locked");
    //     let addresses: Vec<(UfoId, usize, usize)> = core.objects_by_id.values()
    //     .map(|ufo| {
    //         let ufo = ufo.read().expect("ufo locked");
    //         let base = ufo.mmap.as_ptr() as usize;
    //         (ufo.id, base, base + ufo.mmap.length())
    //     } ).collect();
    //     for (id, l, h) in addresses {
    //         if !core.objects_by_segment.contains_key(&l) || !core.objects_by_segment.contains_key(&h.saturating_sub(1)){
    //             core
    //             .objects_by_segment.iter()
    //             .for_each(|e| {
    //                 eprintln!("{:?}: {:#x}-{:#x}", e.value.read().expect("locked ufo").id, e.start, e.end)
    //             });
    //             panic!("{:?} missing! {:#x}-{:#x}", id, l, h);
    //         }
    //     }
    // }

    pub fn new(config: UfoCoreConfig) -> Result<Arc<UfoCore>, std::io::Error> {
        // If this fails then there is nothing we should even try to do about it honestly
        let uffd = userfaultfd::UffdBuilder::new()
            .close_on_exec(true)
            .non_blocking(false)
            .create()
            .unwrap();

        let config = Arc::new(config);
        // We want zero capacity so that when we shut down there isn't a chance of any messages being lost
        // TODO CMYK 2021.03.04: find a way to close the channel but still clear the queue
        let (send, recv) = crossbeam::channel::bounded(0);

        let state = Mutex::new(UfoCoreState {
            object_id_gen: UfoIdGen::new(),

            loaded_chunks: UfoChunks::new(Arc::clone(&config)),
            objects_by_id: HashMap::new(),
            objects_by_segment: IntervalMap::new(),
        });

        let core = Arc::new(UfoCore {
            uffd,
            config,
            msg_send: send,
            // msg_recv: recv,
            state,
        });

        trace!(target: "ufo_core", "starting threads");
        let pop_core = core.clone();
        let pop_workers = PopulateWorkers::new("Ufo Core", move |request_worker| {
            UfoCore::populate_loop(pop_core.clone(), request_worker)
        });
        pop_workers.request_worker();
        PopulateWorkers::spawn_worker(pop_workers.clone());

        // std::thread::Builder::new()
        //     .name("Ufo Core".to_string())
        //     .spawn(move || UfoCore::populate_loop(pop_core))?;

        let msg_core = core.clone();
        std::thread::Builder::new()
            .name("Ufo Msg".to_string())
            .spawn(move || UfoCore::msg_loop(msg_core, pop_workers, recv))?;

        Ok(core)
    }

    pub fn allocate_ufo(
        &self,
        object_config: UfoObjectConfig,
    ) -> Result<WrappedUfoObject, UfoAllocateErr> {
        let (fulfiller, awaiter) = promissory::promissory();
        self.msg_send
            .send(UfoInstanceMsg::Allocate(fulfiller, object_config))
            .expect("Messages pipe broken");

        Ok(awaiter.await_value()?)
    }

    fn get_locked_state(&self) -> anyhow::Result<MutexGuard<UfoCoreState>> {
        match self.state.lock() {
            Err(_) => Err(anyhow::Error::msg("broken lock")),
            Ok(l) => Ok(l),
        }
    }

    fn ensure_capcity(config: &UfoCoreConfig, state: &mut UfoCoreState, to_load: usize) {
        assert!(to_load + config.low_watermark < config.high_watermark);
        if to_load + state.loaded_chunks.used_memory > config.high_watermark {
            state.loaded_chunks.free_until_low_water_mark().unwrap();
        }
    }

    pub fn get_ufo_by_id(&self, id: UfoId) -> Result<WrappedUfoObject, UfoLookupErr> {
        self.get_locked_state()
            .map_err(|e| UfoLookupErr::CoreBroken(format!("{:?}", e)))?
            .objects_by_id
            .get(&id)
            .cloned()
            .map(Ok)
            .unwrap_or_else(|| Err(UfoLookupErr::UfoNotFound))
    }

    pub fn get_ufo_by_address(&self, ptr: usize) -> Result<WrappedUfoObject, UfoLookupErr> {
        self.get_locked_state()
            .map_err(|e| UfoLookupErr::CoreBroken(format!("{:?}", e)))?
            .objects_by_segment
            .get(&ptr)
            .cloned()
            .map(Ok)
            .unwrap_or_else(|| Err(UfoLookupErr::UfoNotFound))
    }

    fn populate_loop(this: Arc<UfoCore>, request_worker: &dyn RequestWorker) {
        trace!(target: "ufo_core", "Started pop loop");
        fn populate_impl(
            core: &UfoCore,
            buffer: &mut UfoWriteBuffer,
            addr: *mut c_void,
        ) -> Result<(), UfoPopulateError> {
            // fn droplockster<T>(_: T){}
            let mut state = core.get_locked_state().unwrap();

            let ptr_int = addr as usize;

            // blindly unwrap here because if we get a message for an address we don't have then it is explodey time
            // clone the arc so we aren't borrowing the state
            let ufo_arc = state.objects_by_segment.get(&ptr_int).unwrap().clone();
            let ufo = ufo_arc.read().unwrap();

            let fault_offset = UfoOffset::from_addr(ufo.deref(), addr);

            let config = &ufo.config;

            let load_size = config.elements_loaded_at_once * config.stride;

            let populate_offset = fault_offset.down_to_nearest_n_relative_to_header(load_size);

            let start = populate_offset.as_index_floor();
            let end = start + config.elements_loaded_at_once;
            let pop_end = min(end, config.element_ct);

            let populate_size = min(
                load_size,
                config.true_size - populate_offset.absolute_offset(),
            );

            debug!(target: "ufo_core", "fault at {}, populate {} bytes at {:#x}",
                start, (pop_end-start) * config.stride, populate_offset.as_ptr_int());

            // Before we perform the load ensure that there is capacity
            UfoCore::ensure_capcity(&core.config, &mut *state, load_size);

            // drop the lock before loading so that UFOs can be recursive
            Mutex::unlock(state);

            let config = &ufo.config;
            let chunk = UfoChunk::new(&ufo_arc, &ufo, populate_offset, populate_size);
            trace!("spin locking {:?}.{}", ufo.id, chunk.offset());
            let chunk_lock = ufo
                .writeback_util
                .chunk_locks
                .spinlock(chunk.offset().chunk_number())
                .map_err(|_| UfoPopulateError)?;

            let raw_data = ufo
                .writeback_util
                .try_readback(&chunk.offset())
                .map(Ok)
                .unwrap_or_else(|| {
                    trace!(target: "ufo_core", "calculate");
                    unsafe {
                        buffer.ensure_capcity(load_size);
                        (config.populate)(start, pop_end, buffer.ptr)?;
                        Ok(&buffer.slice()[0..load_size])
                    }
                })?;
            trace!(target: "ufo_core", "data ready");
            trace!("unlock populate {:?}.{}", ufo.id, chunk.offset());
            chunk_lock.unlock(); // once we've loaded the data the rest is non critical

            unsafe {
                core.uffd
                    .copy(
                        raw_data.as_ptr().cast(),
                        chunk.offset().as_ptr_int() as *mut c_void,
                        populate_size,
                        true,
                    )
                    .expect("unable to populate range");
            }
            trace!(target: "ufo_core", "populated");

            assert!(raw_data.len() == load_size);
            let hash_fulfiller = chunk.hash_fulfiller();

            let mut state = core.get_locked_state().unwrap();
            state.loaded_chunks.add(chunk);
            trace!(target: "ufo_core", "chunk saved");

            // release the lock before calculating the hash so other workers can proceed
            Mutex::unlock(state);

            if !config.should_try_writeback() {
                hash_fulfiller.try_init(None);
            } else {
                // Make sure to take a slice of the raw data. the kernel operates in page sized chunks but the UFO ends where it ends
                let calculated_hash = hash_function(&raw_data[0..populate_size]);
                hash_fulfiller.try_init(Some(calculated_hash));
            }

            Ok(())
        }

        let uffd = &this.uffd;
        // Per-worker buffer
        let mut buffer = UfoWriteBuffer::new();

        loop {
            if ShouldRun::Shutdown == request_worker.await_work() {
                return;
            }
            match uffd.read_event() {
                Ok(Some(event)) => match event {
                    userfaultfd::Event::Pagefault { rw: _, addr } => {
                        request_worker.request_worker(); // while we work someone else waits
                        populate_impl(&*this, &mut buffer, addr).expect("Error during populate");
                    }
                    e => panic!("Recieved an event we did not register for {:?}", e),
                },
                Ok(None) => {
                    /*huh*/
                    warn!(target: "ufo_core", "huh")
                }
                Err(userfaultfd::Error::SystemError(e))
                    if e.as_errno() == Some(nix::errno::Errno::EBADF) =>
                {
                    info!(target: "ufo_core", "closing uffd loop on ebadf");
                    return /*done*/;
                }
                Err(userfaultfd::Error::ReadEof) => {
                    info!(target: "ufo_core", "closing uffd loop");
                    return /*done*/;
                }
                err => {
                    err.expect("uffd read error");
                }
            }
        }
    }

    fn msg_loop<F>(
        this: Arc<UfoCore>,
        populate_pool: Arc<PopulateWorkers<F>>,
        recv: Receiver<UfoInstanceMsg>,
    ) {
        trace!(target: "ufo_core", "Started msg loop");
        fn allocate_impl(
            this: &Arc<UfoCore>,
            config: UfoObjectConfig,
        ) -> anyhow::Result<WrappedUfoObject> {
            info!(target: "ufo_object", "new Ufo {{
                header_size: {},
                stride: {},
                header_size_with_padding: {},
                true_size: {},
    
                elements_loaded_at_once: {},
                element_ct: {},
             }}",
                config.header_size,
                config.stride,

                config.header_size_with_padding,
                config.true_size,

                config.elements_loaded_at_once,
                config.element_ct,
            );

            let ufo = {
                let state = &mut *this.get_locked_state()?;

                let id_map = &state.objects_by_id;
                let id_gen = &mut state.object_id_gen;

                let id = id_gen.next(|k| {
                    trace!(target: "ufo_core", "testing id {:?}", k);
                    !k.is_sentinel() && !id_map.contains_key(k)
                });

                debug!(target: "ufo_core", "allocate {:?}: {} elements with stride {} [pad|header⋮body] [{}|{}⋮{}]",
                    id,
                    config.element_ct,
                    config.stride,
                    config.header_size_with_padding - config.header_size,
                    config.header_size,
                    config.stride * config.element_ct,
                );

                let mmap = BaseMmap::new(
                    config.true_size,
                    &[MemoryProtectionFlag::Read, MemoryProtectionFlag::Write],
                    &[MmapFlag::Anonymous, MmapFlag::Private, MmapFlag::NoReserve],
                    None,
                )
                .expect("Mmap Error");

                let mmap_ptr = mmap.as_ptr();
                let true_size = config.true_size;
                let mmap_base = mmap_ptr as usize;
                let segment = Range {
                    start: mmap_base,
                    end: mmap_base + true_size,
                };

                debug!(target: "ufo_core", "mmapped {:#x} - {:#x}", mmap_base, mmap_base + true_size);

                let writeback = UfoFileWriteback::new(id, &config, this)?;
                this.uffd.register(mmap_ptr.cast(), true_size)?;

                //Pre-zero the header, that isn't part of our populate duties
                if config.header_size_with_padding > 0 {
                    unsafe {
                        this.uffd
                            .zeropage(mmap_ptr.cast(), config.header_size_with_padding, true)
                    }?;
                }

                // let header_offset = config.header_size_with_padding - config.header_size;
                // let body_offset = config.header_size_with_padding;
                let ufo = UfoObject {
                    id,
                    core: Arc::downgrade(this),
                    config,
                    mmap,
                    writeback_util: writeback,
                };

                let ufo = Arc::new(RwLock::new(ufo));

                state.objects_by_id.insert(id, ufo.clone());
                state
                    .objects_by_segment
                    .insert(segment, ufo.clone())
                    .expect("non-overlapping ufos");
                Ok(ufo)
            };

            // this.assert_segment_map();

            ufo
        }

        fn reset_impl(this: &Arc<UfoCore>, ufo_id: UfoId) -> anyhow::Result<()> {
            {
                let state = &mut *this.get_locked_state()?;

                let ufo = &mut *(state
                    .objects_by_id
                    .get(&ufo_id)
                    .map(Ok)
                    .unwrap_or_else(|| Err(anyhow::anyhow!("unknown ufo")))?
                    .write()
                    .map_err(|_| anyhow::anyhow!("lock poisoned"))?);

                debug!(target: "ufo_core", "resetting {:?}", ufo.id);

                ufo.reset_internal()?;

                state.loaded_chunks.drop_ufo_chunks(ufo_id);
            }

            // this.assert_segment_map();

            Ok(())
        }

        fn free_impl(this: &Arc<UfoCore>, ufo_id: UfoId) -> anyhow::Result<()> {
            // this.assert_segment_map();
            {
                let state = &mut *this.get_locked_state()?;
                let ufo = state
                    .objects_by_id
                    .remove(&ufo_id)
                    .map(Ok)
                    .unwrap_or_else(|| Err(anyhow::anyhow!("No such Ufo")))?;
                let ufo = ufo
                    .write()
                    .map_err(|_| anyhow::anyhow!("Broken Ufo Lock"))?;

                debug!(target: "ufo_core", "freeing {:?} @ {:?}", ufo.id, ufo.mmap.as_ptr());

                let mmap_base = ufo.mmap.as_ptr() as usize;
                let segment = state
                    .objects_by_segment
                    .get_entry(&mmap_base)
                    .map(Ok)
                    .unwrap_or_else(|| Err(anyhow::anyhow!("memory segment missing")))?;

                debug_assert_eq!(
                    mmap_base, *segment.start,
                    "mmap lower bound not equal to segment lower bound"
                );
                debug_assert_eq!(
                    mmap_base + ufo.mmap.length(),
                    *segment.end,
                    "mmap upper bound not equal to segment upper bound"
                );

                this.uffd
                    .unregister(ufo.mmap.as_ptr().cast(), ufo.config.true_size)?;
                let start_addr = segment.start.clone();
                state.objects_by_segment.remove_by_start(&start_addr);

                state.loaded_chunks.drop_ufo_chunks(ufo_id);
            }

            // this.assert_segment_map();

            Ok(())
        }

        fn shutdown_impl<F>(this: &Arc<UfoCore>, populate_pool: Arc<PopulateWorkers<F>>) {
            info!(target: "ufo_core", "shutting down");
            let keys: Vec<UfoId> = {
                let state = &mut *this.get_locked_state().expect("err on shutdown");
                state.objects_by_id.keys().cloned().collect()
            };

            keys.iter()
                .for_each(|k| free_impl(this, *k).expect("err on free"));
            populate_pool.shutdown();
        }

        loop {
            match recv.recv() {
                Ok(m) => match m {
                    UfoInstanceMsg::Allocate(fulfiller, cfg) => {
                        fulfiller
                            .fulfill(allocate_impl(&this, cfg).expect("Allocate Error"))
                            .unwrap_or(());
                    }
                    UfoInstanceMsg::Reset(_, ufo_id) => {
                        reset_impl(&this, ufo_id).expect("Reset Error")
                    }
                    UfoInstanceMsg::Free(_, ufo_id) => {
                        free_impl(&this, ufo_id).expect("Free Error")
                    }
                    UfoInstanceMsg::Shutdown(_) => {
                        shutdown_impl(&this, populate_pool);
                        drop(recv);
                        info!(target: "ufo_core", "closing msg loop");
                        return /*done*/;
                    }
                },
                err => {
                    err.expect("recv error");
                }
            }
        }
    }

    pub fn shutdown(&self) {
        let sync = WaitGroup::new();
        trace!(target: "ufo_core", "sending shutdown msg");
        self.msg_send
            .send(UfoInstanceMsg::Shutdown(sync.clone()))
            .expect("Can't send shutdown signal");
        trace!(target: "ufo_core", "awaiting shutdown sync");
        sync.wait();
        trace!(target: "ufo_core", "sync, closing uffd filehandle");

        let fd = std::os::unix::prelude::AsRawFd::as_raw_fd(&self.uffd);
        // this will signal to the populate loop that it is time to close down
        let close_result = unsafe { libc::close(fd) };
        match close_result {
            0 => {}
            _ => {
                panic!(
                    "clouldn't close uffd handle {}",
                    nix::errno::Errno::last().desc()
                );
            }
        }
        trace!(target: "ufo_core", "close uffd handle: {}", close_result);
    }
}
