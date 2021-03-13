// #![feature(ptr_internals, once_cell, slice_ptr_get)]

use std::{io::Error, sync::Arc};
use ufos_core::*;

pub struct UfoCore {
    core: Arc<ufos_core::UfoCore>,
}

impl UfoCore {
    pub fn new_ufo_core(config: UfoCoreConfig) -> Result<UfoCore, Error> {
        let core = ufos_core::UfoCore::new(config)?;
        Ok(UfoCore { core })
    }

    pub fn ufo_prototype(
        header_size: usize,
        stride: usize,
        min_load_ct: Option<usize>,
    ) -> UfoObjectConfigPrototype {
        UfoObjectConfigPrototype::new_prototype(header_size, stride, min_load_ct)
    }

    pub fn new_ufo(
        &self,
        prototype: &UfoObjectConfigPrototype,
        ct: usize,
        populate: Box<UfoPopulateFn>,
    ) -> Result<UfoHandle, UfoAllocateErr> {
        let ufo = self.core.allocate_ufo(prototype.new_config(ct, populate))?;
        Ok(UfoHandle { ufo })
    }
}

impl Drop for UfoCore {
    fn drop(&mut self) {
        self.core.shutdown();
    }
}

pub struct UfoHandle {
    ufo: WrappedUfoObject,
}

// unsafe impl Send for UfoHandle {}

impl UfoHandle {
    pub fn header_ptr(&self) -> Result<*mut std::ffi::c_void, UfoLookupErr> {
        Ok(self.ufo.lock()?.header_ptr())
    }

    pub fn body_ptr(&self) -> Result<*mut std::ffi::c_void, UfoLookupErr> {
        Ok(self.ufo.lock()?.body_ptr())
    }

    pub fn reset(&self) -> Result<(), UfoLookupErr> {
        let waiter = self.ufo.lock()?.reset()?;
        waiter.wait();
        Ok(())
    }

    pub fn free(self) -> Result<(), UfoLookupErr> {
        let waiter = self.ufo.lock()?.free()?;
        waiter.wait();
        Ok(())
    }
}

impl Drop for UfoHandle {
    #[allow(unused_must_use)]
    fn drop(&mut self) {
        // If the lock fails then there is something majorly wrong going on, don't panic inside a panic
        if let Ok(ufo) = self.ufo.lock() {
            ufo.free(); // may have failed if the core is shutdown
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use anyhow::Result;
    use num::Integer;
    use std::{
        convert::{TryFrom, TryInto},
        fmt::Debug,
        mem::size_of,
    };
    use ufos_core::{UfoAllocateErr, UfoCoreConfig};

    #[test]
    fn core_starts() {
        let config = UfoCoreConfig {
            writeback_temp_path: "/tmp".to_string(),
            high_watermark: 1024 * 1024 * 1024,
            low_watermark: 512 * 1024 * 1024,
        };
        let core = UfoCore::new_ufo_core(config).expect("error getting core");

        std::thread::sleep(std::time::Duration::from_millis(100));

        std::mem::drop(core);
    }

    fn basic_test_object<T>(
        header_size: usize,
        body_size: usize,
        min_load: usize,
    ) -> Result<(UfoCore, UfoHandle), UfoAllocateErr>
    where
        T: Sized + Integer + TryFrom<usize>,
        <T as TryFrom<usize>>::Error: Debug,
    {
        let config = UfoCoreConfig {
            writeback_temp_path: "/tmp".to_string(),
            high_watermark: 1024 * 1024 * 1024,
            low_watermark: 512 * 1024 * 1024,
        };
        let core = UfoCore::new_ufo_core(config).expect("error getting core");

        let ufo_prototype =
            UfoObjectConfigPrototype::new_prototype(header_size, size_of::<T>(), Some(min_load));

        let o = core.new_ufo(
            &ufo_prototype,
            body_size,
            Box::new(|start, end, fill| {
                let slice = unsafe {
                    std::slice::from_raw_parts_mut::<T>(fill.cast(), size_of::<T>() * (end - start))
                };
                for idx in start..end {
                    slice[idx - start] = idx.try_into().unwrap();
                }

                Ok(())
            }),
        )?;

        Ok((core, o))
    }

    #[test]
    fn it_works() -> Result<(), UfoAllocateErr> {
        let (core, o) = basic_test_object::<u32>(0, 1000 * 1000, 4096)?;

        let arr = unsafe {
            std::slice::from_raw_parts_mut(o.body_ptr().unwrap().cast::<u32>(), 1000 * 1000)
        };

        for x in 0..1000 * 1000 {
            assert_eq!(x as u32, arr[x]);
        }

        std::mem::drop(core);
        Ok(())
    }

    #[test]
    fn with_header() -> Result<(), UfoAllocateErr> {
        let (core, o) = basic_test_object::<u32>(1, 1000 * 1000, 4096)?;

        unsafe { assert_eq!(*o.header_ptr().unwrap().cast::<u32>(), 0) };

        let arr = unsafe {
            std::slice::from_raw_parts_mut(o.body_ptr().unwrap().cast::<u32>(), 1000 * 1000)
        };

        for x in 0..1000 * 1000 {
            assert_eq!(x as u32, arr[x]);
        }

        std::mem::drop(core);
        Ok(())
    }

    #[test]
    fn reverse_iterate() -> Result<(), UfoAllocateErr> {
        let ct = 1000 * 1000;
        let (core, o) = basic_test_object::<u32>(1, ct, 4096)?;

        unsafe { assert_eq!(*o.header_ptr().unwrap().cast::<u32>(), 0) };

        let arr = unsafe {
            std::slice::from_raw_parts_mut(
                o.body_ptr().unwrap().cast::<u32>(),
                size_of::<u32>() * 1000 * 1000,
            )
        };

        for x in 1..=ct {
            let y = ct - x;
            assert_eq!(y as u32, arr[y]);
        }

        std::mem::drop(core);
        Ok(())
    }

    #[test]
    fn large_load() -> anyhow::Result<()> {
        // use stderrlog;
        // stderrlog::new()
        //     // .module("ufo_core")
        //     .verbosity(4)
        //     .timestamp(stderrlog::Timestamp::Microsecond)
        //     .init()
        //     .unwrap();

        let ct = 1000 * 1000 * 2000;
        let (core, o) = basic_test_object::<u64>(0, ct, 1024 * 1024)?;

        let arr =
            unsafe { std::slice::from_raw_parts_mut(o.body_ptr().unwrap().cast::<u64>(), ct) };

        for x in 0..ct {
            if !(x as u64 == arr[x]) {
                anyhow::bail!("{} != {}", x, arr[x]);
            }
        }

        std::mem::drop(core);
        Ok(())
    }

    #[test]
    fn large_write() -> anyhow::Result<()> {
        // use stderrlog;
        // stderrlog::new()
        //     // .module("ufo_core")
        //     .verbosity(4)
        //     .timestamp(stderrlog::Timestamp::Millisecond)
        //     .init()
        //     .unwrap();

        let ct = 1000 * 1000 * 200;
        let (core, o) = basic_test_object::<u64>(0, ct, 1024 * 1024)?;

        let arr =
            unsafe { std::slice::from_raw_parts_mut(o.body_ptr().unwrap().cast::<u64>(), ct) };

        arr[0] = 14;

        for x in 1..ct {
            if !(x as u64 == arr[x]) {
                anyhow::bail!("{} != {}", x, arr[x]);
            }
        }

        assert_eq!(14, arr[0]);

        std::mem::drop(core);
        Ok(())
    }

    #[test]
    fn reset_ufo() -> anyhow::Result<()> {
        // use stderrlog;
        // stderrlog::new()
        //     // .module("ufo_core")
        //     .verbosity(4)
        //     .timestamp(stderrlog::Timestamp::Microsecond)
        //     .init()
        //     .unwrap();

        let ct = 1024 * 1024 * 200;
        let (core, o) = basic_test_object::<u64>(0, ct, 4096 * 4)?;

        let arr =
            unsafe { std::slice::from_raw_parts_mut(o.body_ptr().unwrap().cast::<u64>(), ct) };

        for x in 0..ct {
            if x as u64 != arr[x] {
                anyhow::bail!("  {} != {}", x, arr[x]);
            }
        }

        for x in 0..ct {
            arr[x] = 7;
        }

        for x in 0..ct {
            if 7 != arr[x] {
                anyhow::bail!("  7 != {} @ {}", arr[x], x);
            }
        }

        for x in 0..ct {
            arr[x] = 8;
        }

        for x in 0..ct {
            if 8 != arr[x] {
                anyhow::bail!("  7 != {} @ {}", arr[x], x);
            }
        }

        o.reset()?;

        for x in 0..ct {
            if x as u64 != arr[x] {
                anyhow::bail!("p {} != {}", x, arr[x]);
            }
        }

        std::mem::drop(core);
        Ok(())
    }
}
