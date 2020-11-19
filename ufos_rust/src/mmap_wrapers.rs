use std::alloc;
use std::cell::RefCell;

use libc;
use std::io::Error;
use std::lazy::SyncLazy;
use std::num::NonZeroUsize;
use std::ops::Deref;
use std::ptr::NonNull;
use std::rc::{Rc, Weak};
use std::result::Result;

// #[cfg(target_os = "linux")]
use std::os::unix::io::RawFd;

fn check_return_zero(result: i32) -> Result<(), Error> {
    match result {
        0 => Ok(()),
        -1 => Err(Error::last_os_error()),
        _ => panic!("return other than 0 or -1 ({})", result),
    }
}

fn check_return_nonneg(result: i32) -> Result<i32, Error> {
    match result {
        -1 => Err(Error::last_os_error()),
        x if x < -1 => panic!("return other than 0 or -1 ({})", result),
        x => Ok(x),
    }
}

pub trait Mmap {
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
            let arr = unsafe {
                std::slice::from_raw_parts(self.as_ptr().offset(offset as isize), length)
            };
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
            let arr = unsafe {
                std::slice::from_raw_parts_mut(self.as_ptr().offset(offset as isize), length)
            };
            Some(f(arr))
        }
    }
}

#[repr(i32)]
#[derive(Debug, Copy, Clone)]
pub enum MemoryProtectionFlag {
    Exec = libc::PROT_EXEC,
    Read = libc::PROT_READ,
    Write = libc::PROT_WRITE,
    None = libc::PROT_NONE,
}

#[repr(i32)]
#[derive(Debug, Copy, Clone)]
pub enum MmapFlag {
    Shared = libc::MAP_SHARED,
    SharedValidate = libc::MAP_SHARED_VALIDATE,
    Private = libc::MAP_PRIVATE,
    B32 = libc::MAP_32BIT,
    Annon = libc::MAP_ANON,
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
        let (fd, offset) = fd_offset.unwrap_or((0, 0));

        let ptr = unsafe { libc::mmap64(std::ptr::null_mut(), length, prot, flags, fd, offset) };

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
}

impl Drop for BaseMmap {
    fn drop(&mut self) {
        unsafe {
            libc::munmap(self.base.cast(), self.length());
        }
    }
}

pub struct OpenFile {
    fd: RawFd
}

impl OpenFile {
    pub unsafe fn temp(path: &str, size: usize) -> Result<Self, Error>{
        let tmp = std::ffi::CString::new(path)?;

        let fd = check_return_nonneg(libc::open64(
            tmp.as_ptr(),
            libc::O_RDWR | libc::O_TMPFILE,
            0600,
        ))?;

        // make this before we truncate to a larger size so the file is closed even on error
        let f = OpenFile{fd};

        // truncate the file to the needed size
        check_return_zero(libc::ftruncate64(fd, size as i64))?;

        Ok(f)
    }

    pub fn as_fd(&self) -> RawFd{
        self.fd
    }
}

impl Drop for OpenFile {
    fn drop(&mut self) {
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
        Ok(MmapFd { mmap, fd })
    }
}

impl Mmap for MmapFd {
    fn as_ptr(&self) -> *mut u8 {
        self.mmap.as_ptr()
    }

    fn length(&self) -> usize {
        self.mmap.length()
    }
}

