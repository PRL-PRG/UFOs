use std::io::Error;
use std::lazy::SyncLazy;
use std::num::NonZeroUsize;
use std::sync::{
    atomic::{AtomicU8, Ordering},
    Arc, RwLock, RwLockReadGuard, Weak,
};

use anyhow::Result;
use crossbeam::sync::WaitGroup;
use thiserror::Error;

use log::{debug, error, trace};

use crate::bitwise_spinlock::Bitlock;
use crate::mmap_wrapers;
use crate::once_await::OnceAwait;
use crate::once_await::OnceFulfiller;

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

pub fn hash_function(data: &[u8]) -> DataHash {
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
    pub read_only: bool,
}

impl UfoObjectConfigPrototype {
    pub fn new_prototype(
        header_size: usize,
        stride: usize,
        min_load_ct: Option<usize>,
        read_only: bool,
    ) -> UfoObjectConfigPrototype {
        UfoObjectConfigPrototype {
            header_size,
            stride,
            min_load_ct,
            read_only,
        }
    }

    pub fn new_config(&self, ct: usize, populate: Box<UfoPopulateFn>) -> UfoObjectConfig {
        UfoObjectConfig::new_config(
            self.header_size,
            ct,
            self.stride,
            self.read_only,
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
    pub(crate) read_only: bool,
}

impl UfoObjectConfig {
    pub(crate) fn new_config(
        header_size: usize,
        element_ct: usize,
        stride: usize,
        read_only: bool,
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
            read_only,

            header_size_with_padding,
            true_size,

            elements_loaded_at_once,
            element_ct,

            populate,
        }
    }

    pub(crate) fn should_try_writeback(&self) -> bool {
        // this may get more complex in the future, for example we may implement ALWAYS writeback
        !self.read_only
    }
}

pub(crate) struct UfoOffset {
    base_addr: usize,
    chunk_number: usize,
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

        let offset_from_header = absolute_offset_bytes - header_bytes;
        let bytes_loaded_at_once = ufo.config.elements_loaded_at_once * ufo.config.stride;
        let chunk_number = offset_from_header.div_floor(bytes_loaded_at_once);
        assert!(chunk_number * bytes_loaded_at_once <= offset_from_header);
        assert!((chunk_number + 1) * bytes_loaded_at_once > offset_from_header);

        UfoOffset {
            base_addr,
            chunk_number,
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
        self.offset_from_header().div_floor(self.stride)
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

    pub fn chunk_number(&self) -> usize {
        self.chunk_number
    }
}

impl std::fmt::Display for UfoOffset {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "(UfoOffset {}/{})",
            self.absolute_offset(),
            self.chunk_number()
        )
    }
}

pub(crate) struct ChunkFreer {
    pivot: Option<BaseMmap>,
}

impl ChunkFreer {
    pub fn new() -> Self {
        ChunkFreer { pivot: None }
    }

    fn ensure_capcity(&mut self, to_fit: &UfoChunk) -> Result<&BaseMmap> {
        let required_size = to_fit.size_in_pages().size_as_multiple_of_pages();
        trace!(target: "ufo_object", "ensuring pivot capacity {}", required_size);
        if let None = self.pivot {
            trace!(target: "ufo_object", "init pivot {}", required_size);
            self.pivot = Some(BaseMmap::new(
                required_size,
                &[MemoryProtectionFlag::Read, MemoryProtectionFlag::Write],
                &[MmapFlag::Anonymous, MmapFlag::Private],
                None,
            )?);
        }

        let current_size = self.pivot.as_ref().expect("just checked").length();
        if current_size < required_size {
            trace!(target: "ufo_object", "grow pivot from {} to {}", current_size, required_size);
            let mut old_pivot = None;
            std::mem::swap(&mut old_pivot, &mut self.pivot);
            let pivot = old_pivot.unwrap();
            self.pivot = Some(pivot.resize(required_size)?);
        }

        Ok(&self.pivot.as_ref().expect("just checked"))
    }

    pub fn free_chunk(&mut self, chunk: &mut UfoChunk) -> Result<usize> {
        if 0 == chunk.size() {
            return Ok(0);
        }
        chunk.free_and_writeback_dirty(self.ensure_capcity(chunk)?)
    }
}

pub(self) struct SizeInPages(usize);

impl SizeInPages {
    fn size_as_multiple_of_pages(&self) -> usize {
        up_to_nearest(self.0, *PAGE_SIZE)
    }
}

pub(crate) struct UfoChunk {
    ufo_id: UfoId,
    object: Weak<RwLock<UfoObject>>,
    offset: UfoOffset,
    length: Option<NonZeroUsize>,
    hash: Arc<OnceAwait<Option<DataHash>>>,
}

impl UfoChunk {
    pub fn new(
        arc: &WrappedUfoObject,
        object: &RwLockReadGuard<UfoObject>,
        offset: UfoOffset,
        length: usize,
    ) -> UfoChunk {
        assert!(
            offset.absolute_offset() + length <= object.mmap.length(),
            "{} + {} > {}",
            offset.absolute_offset(),
            length,
            object.mmap.length()
        );
        UfoChunk {
            ufo_id: object.id,
            object: Arc::downgrade(arc),
            offset,
            length: NonZeroUsize::new(length),
            hash: Arc::new(OnceAwait::new()),
        }
    }

    pub fn offset(&self) -> &UfoOffset {
        &self.offset
    }

    pub fn hash_fulfiller(&self) -> impl OnceFulfiller<Option<DataHash>> {
        self.hash.clone()
    }

    pub fn free_and_writeback_dirty(&mut self, pivot: &BaseMmap) -> Result<usize> {
        match (self.length, self.object.upgrade()) {
            (Some(length), Some(obj)) => {
                let length_bytes = length.get();
                let obj = obj.read().unwrap();

                trace!(target: "ufo_object", "free chunk {:?}@{} ({}b)",
                    self.ufo_id, self.offset.absolute_offset() , length_bytes
                );

                if !obj.config.should_try_writeback() {
                    trace!(target: "ufo_object", "no writeback {:?}", self.ufo_id);
                    // Not doing writebacks, punch it out and leave
                    unsafe {
                        let data_ptr = obj.mmap.as_ptr().add(self.offset.absolute_offset());
                        check_return_zero(libc::madvise(
                            data_ptr.cast(),
                            length_bytes,
                            libc::MADV_DONTNEED,
                        ))?;
                    }
                    return Ok(length_bytes);
                }

                // We first remap the pages from the UFO into the file backing
                // we check the value of the page after the remap because the remap
                //  is atomic and will let us read cleanly in the face of racing writers
                let chunk_number = self.offset.chunk_number();
                trace!("try to uncontended-lock {:?}.{}", obj.id, self.offset());
                let chunk_lock = obj
                    .writeback_util
                    .chunk_locks
                    .lock_uncontended(chunk_number)?;
                unsafe {
                    let length_page_multiple = self.size_in_pages().size_as_multiple_of_pages();
                    anyhow::ensure!(length_page_multiple <= pivot.length(), "Pivot too small");
                    let data_ptr = obj.mmap.as_ptr().add(self.offset.absolute_offset());
                    let pivot_ptr = pivot.as_ptr();
                    check_ptr_nonneg(libc::mremap(
                        data_ptr.cast(),
                        length_page_multiple,
                        length_page_multiple,
                        libc::MREMAP_FIXED | libc::MREMAP_MAYMOVE | libc::MREMAP_DONTUNMAP,
                        pivot_ptr,
                    ))?;
                    trace!(target: "ufo_object", "{:?} mremaped data to pivot", self.ufo_id);
                }

                if let Some(hash) = self.hash.get() {
                    let calculated_hash = pivot.with_slice(0, length_bytes, hash_function).unwrap(); // it should never be possible for this to fail
                    trace!(target: "ufo_object", "writeback hash matches {}", hash == &calculated_hash);
                    if hash != &calculated_hash {
                        pivot.with_slice(0, length_bytes, |data| {
                            obj.writeback_util.writeback(&self.offset, data)
                        });
                    }
                }

                self.length = None;
                trace!("unlock free {:?}.{}", obj.id, self.offset());
                chunk_lock.unlock();
                Ok(length_bytes)
            }
            _ => Ok(0),
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

    pub(self) fn size_in_pages(&self) -> SizeInPages {
        SizeInPages(self.size())
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
    chunk_ct: usize,
    pub(crate) chunk_locks: Bitlock,
    chunk_size: usize,
    total_bytes: usize,
    header_bytes: usize,
    // bitlock_bytes: usize,
    // bitmap_bytes: usize,
}

// someday make this atomic_from_mut
fn atomic_bitset(target: &mut u8, mask: u8) {
    unsafe {
        let t = &mut *(target as *mut u8 as *mut AtomicU8);
        t.fetch_update(Ordering::Relaxed, Ordering::Relaxed, |x| Some(x | mask))
            .unwrap();
    }
}

impl UfoFileWriteback {
    pub fn new(
        ufo_id: UfoId,
        cfg: &UfoObjectConfig,
        core: &Arc<UfoCore>,
    ) -> Result<UfoFileWriteback, Error> {
        let page_size = *PAGE_SIZE;

        let chunk_ct = cfg.element_ct.div_ceil(cfg.elements_loaded_at_once);
        assert!(chunk_ct * cfg.elements_loaded_at_once >= cfg.element_ct);

        let chunk_size = cfg.elements_loaded_at_once * cfg.stride;

        let bitmap_bytes = chunk_ct.div_ceil(8); /*8 bits per byte*/
        // Now we want to get the bitmap bytes up to the next multiple of the page size
        let bitmap_bytes = up_to_nearest(bitmap_bytes, page_size);
        assert!(bitmap_bytes * 8 >= chunk_ct);
        assert!(bitmap_bytes.trailing_zeros() >= page_size.trailing_zeros());

        // the bitlock uses the same math as the bitmap
        let bitlock_bytes = bitmap_bytes;

        // round the mmap up to the nearest chunk size
        // when loading we need to give back chunks this large so even though no useful user data may
        // be in the last chunk we still need to have this available for in the readback chunk
        let data_bytes = up_to_nearest(cfg.element_ct * cfg.stride, chunk_size);
        let total_bytes = bitmap_bytes + bitlock_bytes + data_bytes;

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

        let chunk_locks = Bitlock::new(unsafe { mmap.as_ptr().add(bitmap_bytes) }, chunk_ct);

        Ok(UfoFileWriteback {
            ufo_id,
            chunk_ct,
            chunk_size,
            chunk_locks,
            mmap,
            total_bytes,
            header_bytes: bitmap_bytes + bitlock_bytes,
        })
    }

    fn body_bytes(&self) -> usize {
        self.total_bytes - self.header_bytes
    }

    pub(self) fn writeback(&self, offset: &UfoOffset, data: &[u8]) -> Result<()> {
        let off_head = offset.offset_from_header();
        if off_head > self.body_bytes() {
            anyhow::bail!("{} outside of range", off_head);
        }

        let chunk_number = offset.chunk_number();
        assert!(chunk_number < self.chunk_ct);
        assert_eq!(off_head.div_floor(self.chunk_size), chunk_number);
        let writeback_offset = self.header_bytes + off_head;

        let chunk_byte = chunk_number >> 3;
        let chunk_bit = 1u8 << (chunk_number & 0b111);
        assert!(chunk_byte < (self.header_bytes >> 1));

        debug!(target: "ufo_object", "writeback offset {:#x}", writeback_offset);

        let bitmap_ptr: &mut u8 = unsafe { self.mmap.as_ptr().add(chunk_byte).as_mut().unwrap() };
        let expected_size = std::cmp::min(self.chunk_size, self.total_bytes - writeback_offset);

        anyhow::ensure!(
            data.len() == expected_size,
            "given data does not match the expected size"
        );

        // TODO: blocks CAN be loaded with the UFO lock held!! FIXME
        // We aren't a mutable copy but writebacks never overlap and we hold the UFO read lock so a chunk cannot be loaded
        let writeback_arr: &mut [u8] = unsafe {
            std::slice::from_raw_parts_mut(self.mmap.as_ptr().add(writeback_offset), expected_size)
        };

        writeback_arr.copy_from_slice(data);
        atomic_bitset(bitmap_ptr, chunk_bit);

        Ok(())
    }

    pub fn try_readback<'a>(&'a self, offset: &UfoOffset) -> Option<&'a [u8]> {
        let off_head = offset.offset_from_header();
        trace!(target: "ufo_object", "try readback {:?}@{:#x}", self.ufo_id, off_head);

        let chunk_number = off_head.div_floor(self.chunk_size);
        let readback_offset = self.header_bytes + off_head;

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

impl std::cmp::PartialEq for UfoObject {
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
    }
}

impl std::cmp::Eq for UfoObject {}

impl UfoObject {
    pub(crate) fn reset_internal(&mut self) -> anyhow::Result<()> {
        let length = self.config.true_size - self.config.header_size_with_padding;
        unsafe {
            check_return_zero(libc::madvise(
                self.mmap
                    .as_ptr()
                    .add(self.config.header_size_with_padding)
                    .cast(),
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

    pub fn reset(&mut self) -> Result<WaitGroup, UfoLookupErr> {
        let wait_group = crossbeam::sync::WaitGroup::new();
        let core = match self.core.upgrade() {
            None => return Err(UfoLookupErr::CoreShutdown),
            Some(x) => x,
        };

        core.msg_send
            .send(UfoInstanceMsg::Reset(wait_group.clone(), self.id))?;

        Ok(wait_group)
    }

    pub fn free(&mut self) -> Result<WaitGroup, UfoLookupErr> {
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
