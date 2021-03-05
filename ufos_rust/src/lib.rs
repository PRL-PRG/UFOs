#![feature(ptr_internals, once_cell, slice_ptr_get)]

mod math;
mod mmap_wrapers;
mod return_checks;
mod ufo_core;
mod ufo_objects;

use promissory::Awaiter;
pub use ufo_core::UfoCoreConfig;
use ufo_core::UfoInstanceMsg;
pub use ufo_objects::{UfoHandle, UfoObjectConfig, UfoObjectConfigPrototype, UfoPopulateFn};

use std::io::Error;
use std::sync::Arc;

pub struct UfoCore {
    core: Arc<ufo_core::UfoCore>,
}

impl UfoCore {
    pub fn new_ufo_core(config: UfoCoreConfig) -> Result<UfoCore, Error> {
        let core = ufo_core::UfoCore::new(config)?;
        Ok(UfoCore { core })
    }

    pub fn ufo_prototype(
        header_size: usize,
        stride: usize,
        min_load_ct: Option<usize>,
    ) -> UfoObjectConfigPrototype {
        UfoObjectConfigPrototype::new_prototype(header_size, stride, min_load_ct)
    }

    pub fn allocate_ufo(&self, object_config: UfoObjectConfig) -> UfoHandle {
        let (fulfiller, awaiter) = promissory::promissory();
        self.core
            .msg_send
            .send(UfoInstanceMsg::Allocate(fulfiller, object_config))
            .expect("Messages pipe broken");

        awaiter.await_value()
    }

    pub fn shutdown(self) {
        self.core.shutdown();
    }
}

#[cfg(test)]
mod tests {
    use crate::{UfoCore, UfoCoreConfig, UfoHandle, UfoObjectConfig};
    use num::Integer;
    use std::{
        convert::{TryFrom, TryInto},
        fmt::Debug,
        mem::size_of,
    };

    #[test]
    fn core_starts() {
        let config = UfoCoreConfig {
            writeback_temp_path: "/tmp",
            high_watermark: 1024 * 1024 * 1024,
            low_watermark: 512 * 1024 * 1024,
        };
        let core = UfoCore::new_ufo_core(config).expect("error getting core");

        std::thread::sleep(std::time::Duration::from_millis(100));

        core.shutdown();
    }

    fn basic_test_object<T>(
        header_size: usize,
        body_size: usize,
        min_load: usize,
    ) -> (UfoCore, UfoHandle)
    where
        T: Sized + Integer + TryFrom<usize>,
        <T as TryFrom<usize>>::Error: Debug,
    {
        let config = UfoCoreConfig {
            writeback_temp_path: "/tmp",
            high_watermark: 1024 * 1024 * 1024,
            low_watermark: 512 * 1024 * 1024,
        };
        let core = UfoCore::new_ufo_core(config).expect("error getting core");

        let obj_cfg = UfoObjectConfig::new_config(
            header_size,
            body_size,
            size_of::<T>(),
            Some(min_load),
            Box::new(|start, end, fill| {
                let slice = unsafe {
                    std::slice::from_raw_parts_mut::<T>(fill.cast(), size_of::<T>() * (end - start))
                };
                for idx in start..end {
                    slice[idx - start] = idx.try_into().unwrap();
                }
            }),
        );

        let o = core.allocate_ufo(obj_cfg);

        (core, o)
    }

    #[test]
    fn it_works() {
        let (core, o) = basic_test_object::<u32>(0, 1000 * 1000, 4096);

        let arr =
            unsafe { std::slice::from_raw_parts_mut(o.body_ptr().cast::<u32>(), 1000 * 1000) };

        for x in 0..1000 * 1000 {
            assert_eq!(x as u32, arr[x]);
        }

        core.shutdown();
    }

    #[test]
    fn with_header() {
        let (core, o) = basic_test_object::<u32>(1, 1000 * 1000, 4096);

        unsafe { assert_eq!(*o.header_ptr().cast::<u32>(), 0) };

        let arr =
            unsafe { std::slice::from_raw_parts_mut(o.body_ptr().cast::<u32>(), 1000 * 1000) };

        for x in 0..1000 * 1000 {
            assert_eq!(x as u32, arr[x]);
        }

        core.shutdown();
    }

    #[test]
    fn reverse_iterate() {
        let (core, o) = basic_test_object::<u32>(1, 1000 * 1000, 4096);

        unsafe { assert_eq!(*o.header_ptr().cast::<u32>(), 0) };

        let arr = unsafe {
            std::slice::from_raw_parts_mut(
                o.body_ptr().cast::<u32>(),
                size_of::<u32>() * 1000 * 1000,
            )
        };

        for x in 1..=1000 * 1000 {
            let y = 1000 * 1000 - x;
            assert_eq!(y as u32, arr[y]);
        }

        core.shutdown();
    }

    #[test]
    fn large_load() -> anyhow::Result<()> {
        use stderrlog;
        stderrlog::new()
            // .module("ufo_core")
            .verbosity(4)
            .timestamp(stderrlog::Timestamp::Microsecond)
            .init()
            .unwrap();

        let ct = 1000 * 1000 * 2000;
        let (core, o) = basic_test_object::<u64>(0, ct, 1024 * 1024);

        let arr = unsafe { std::slice::from_raw_parts_mut(o.body_ptr().cast::<u64>(), ct) };

        for x in 0..ct {
            if !(x as u64 == arr[x]) {
                anyhow::bail!("{} != {}", x, arr[x]);
            }
        }

        core.shutdown();
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
        let (core, o) = basic_test_object::<u64>(0, ct, 1024 * 1024);

        let arr = unsafe { std::slice::from_raw_parts_mut(o.body_ptr().cast::<u64>(), ct) };

        arr[0] = 14;

        for x in 1..ct {
            if !(x as u64 == arr[x]) {
                anyhow::bail!("{} != {}", x, arr[x]);
            }
        }

        assert_eq!(14, arr[0]);

        core.shutdown();
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
        let (core, o) = basic_test_object::<u64>(0, ct, 4096 * 4);

        let arr = unsafe { std::slice::from_raw_parts_mut(o.body_ptr().cast::<u64>(), ct) };

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

        o.reset()?;

        for x in 0..ct {
            if x as u64 != arr[x] {
                anyhow::bail!("p {} != {}", x, arr[x]);
            }
        }

        core.shutdown();
        Ok(())
    }
}
