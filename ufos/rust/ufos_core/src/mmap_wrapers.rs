use std::io::Error;
use std::result::Result;

use std::os::unix::io::RawFd;

use super::return_checks::*;
use log::debug;

static PAGE_SIZE: std::lazy::SyncOnceCell<usize> = std::lazy::SyncOnceCell::new();

pub fn get_page_size() -> usize {
    *PAGE_SIZE.get_or_init(|| {
        let ps = unsafe { libc::sysconf(libc::_SC_PAGESIZE) };
        assert!(ps > 0);
        ps as usize
    })
}

pub trait Mmap: Sized {
    fn as_ptr(&self) -> *mut u8;

    fn length(&self) -> usize;

    fn with_slice<F, V>(&self, offset: usize, length: usize, f: F) -> Option<V>
    where
        F: FnOnce(&[u8]) -> V,
    {
        let end = offset + length;

        if end > self.length() {
            None
        } else {
            let arr = unsafe { std::slice::from_raw_parts(self.as_ptr().add(offset), length) };
            Some(f(arr))
        }
    }

    fn with_slice_mut<F, V>(&mut self, offset: usize, length: usize, f: F) -> Option<V>
    where
        F: FnOnce(&mut [u8]) -> V,
    {
        let end = offset + length;

        if end > self.length() {
            None
        } else {
            let arr = unsafe { std::slice::from_raw_parts_mut(self.as_ptr().add(offset), length) };
            Some(f(arr))
        }
    }

    fn resize(self, new_size: usize) -> Result<Self, Error>;
}

#[repr(i32)]
#[derive(Debug, Copy, Clone)]
#[allow(dead_code)]
pub enum MemoryProtectionFlag {
    Exec = libc::PROT_EXEC,
    Read = libc::PROT_READ,
    Write = libc::PROT_WRITE,
    None = libc::PROT_NONE,
}

#[repr(i32)]
#[derive(Debug, Copy, Clone)]
#[allow(dead_code)]
pub enum MmapFlag {
    Shared = libc::MAP_SHARED,
    SharedValidate = libc::MAP_SHARED_VALIDATE,
    Private = libc::MAP_PRIVATE,
    B32 = libc::MAP_32BIT,
    Anonymous = libc::MAP_ANONYMOUS,
    Fixed = libc::MAP_FIXED,
    FixedNoreplae = libc::MAP_FIXED_NOREPLACE,
    GrowsDown = libc::MAP_GROWSDOWN,
    HugeTlb = libc::MAP_HUGETLB,
    Locked = libc::MAP_LOCKED,
    NoReserve = libc::MAP_NORESERVE,
    Populate = libc::MAP_POPULATE,
    Stack = libc::MAP_STACK,
    Sync = libc::MAP_SYNC,
}

pub struct BaseMmap {
    base: *mut u8,
    len: usize,
}

unsafe impl Send for BaseMmap {}
unsafe impl Sync for BaseMmap {}

impl BaseMmap {
    fn new0(
        length: usize,
        memory_protection: &[MemoryProtectionFlag],
        mmap_flags: &[MmapFlag],
        huge_pagesize_log2: Option<u8>,
        fd_offset: Option<(RawFd, i64)>,
    ) -> Result<BaseMmap, Error> {
        // If we were given a huge page size get it into the right shape
        let huge_tlb_size = huge_pagesize_log2
            .map(|x| (x as i32) << libc::MAP_HUGE_SHIFT)
            .unwrap_or(0);

        if huge_tlb_size != (huge_tlb_size & libc::MAP_HUGE_MASK) {
            panic!("bad TLB size given {}", huge_pagesize_log2.unwrap());
        }

        // Fold the bit fields
        let prot = memory_protection.iter().fold(0, |a, b| a | *b as i32);
        let flags = mmap_flags.iter().fold(0, |a, b| a | *b as i32);
        let flags = flags | huge_tlb_size;
        // zeros are traditional gifts when you don't know what to give
        let (fd, offset) = fd_offset.unwrap_or((-1, 0));

        debug!(target: "ufo_malloc", "malloc {} bytes on fd {} with offset {}", length, fd, offset);
        let ptr = unsafe { libc::mmap(std::ptr::null_mut(), length, prot, flags, fd, offset) };

        if ptr == libc::MAP_FAILED {
            return Err(Error::last_os_error());
        }

        Ok(BaseMmap {
            base: ptr.cast(),
            len: length,
        })
    }

    pub fn new(
        length: usize,
        memory_protection: &[MemoryProtectionFlag],
        mmap_flags: &[MmapFlag],
        huge_pagesize_log2: Option<u8>,
    ) -> Result<BaseMmap, Error> {
        BaseMmap::new0(
            length,
            memory_protection,
            mmap_flags,
            huge_pagesize_log2,
            None,
        )
    }

    pub fn as_ptr(&self) -> *mut u8 {
        self.base
    }
}

impl Mmap for BaseMmap {
    fn as_ptr(&self) -> *mut u8 {
        self.base
    }

    fn length(&self) -> usize {
        self.len
    }

    fn resize(mut self, new_size: usize) -> Result<Self, Error> {
        unsafe {
            let new_ptr = check_ptr_nonneg(libc::mremap(
                self.as_ptr().cast(),
                self.length(),
                new_size,
                libc::MREMAP_MAYMOVE,
            ))?;
            self.base = new_ptr.cast();
        }
        self.len = new_size;
        Ok(self)
    }
}

impl Drop for BaseMmap {
    fn drop(&mut self) {
        debug!(target: "ufo_malloc", "free malloc {:#x}", self.base as usize);
        unsafe {
            libc::munmap(self.base.cast(), self.length());
        }
    }
}

pub struct OpenFile {
    fd: RawFd,
}

impl OpenFile {
    pub unsafe fn temp(path: &str, size: usize) -> Result<Self, Error> {
        let tmp = std::ffi::CString::new(path)?;

        debug!(target: "ufo_malloc", "open anonymous temporary file at {}", path);
        let fd = check_return_nonneg(libc::open(
            tmp.as_ptr(),
            libc::O_RDWR | libc::O_TMPFILE,
            0o600,
        ))?;

        // make this before we truncate to a larger size so the file is closed even on error
        let f = OpenFile { fd };
        debug!(target: "ufo_malloc", "opened file {}", fd);

        // truncate the file to the needed size
        check_return_zero(libc::ftruncate64(fd, size as i64))?;

        Ok(f)
    }

    pub fn as_fd(&self) -> RawFd {
        self.fd
    }
}

impl Drop for OpenFile {
    fn drop(&mut self) {
        debug!(target: "ufo_malloc", "close file {}", self.fd);
        unsafe { libc::close(self.fd) };
    }
}

pub struct MmapFd {
    mmap: BaseMmap,
    _fd: OpenFile,
}

impl MmapFd {
    pub fn new(
        length: usize,
        memory_protection: &[MemoryProtectionFlag],
        mmap_flags: &[MmapFlag],
        huge_pagesize_log2: Option<u8>,
        fd: OpenFile,
        offset: i64,
    ) -> Result<MmapFd, Error> {
        let mmap = BaseMmap::new0(
            length,
            memory_protection,
            mmap_flags,
            huge_pagesize_log2,
            Some((fd.as_fd(), offset)),
        )?;
        Ok(MmapFd { mmap, _fd: fd })
    }
}

impl Mmap for MmapFd {
    fn as_ptr(&self) -> *mut u8 {
        self.mmap.as_ptr()
    }

    fn length(&self) -> usize {
        self.mmap.length()
    }

    fn resize(mut self, new_size: usize) -> Result<Self, Error> {
        self.mmap = self.mmap.resize(new_size)?;
        Ok(self)
    }
}
