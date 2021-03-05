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
    use std::mem::size_of;

    use crate::{UfoCore, UfoCoreConfig, UfoObjectConfig};

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

    #[test]
    fn it_works() {
        let config = UfoCoreConfig {
            writeback_temp_path: "/tmp",
            high_watermark: 1024 * 1024 * 1024,
            low_watermark: 512 * 1024 * 1024,
        };
        let core = UfoCore::new_ufo_core(config).expect("error getting core");

        let obj_cfg = UfoObjectConfig::new_config(
            0,
            1000 * 1000,
            size_of::<u32>(),
            Some(4096),
            Box::new(|start, end, fill| {
                let slice = unsafe {
                    std::slice::from_raw_parts_mut(fill.cast(), size_of::<u32>() * (end - start))
                };
                for idx in start..end {
                    slice[idx - start] = idx as u32;
                }
            }),
        );

        let o = core.allocate_ufo(obj_cfg);

        let arr = unsafe {
            std::slice::from_raw_parts_mut(
                o.body_ptr().cast::<u32>(),
                size_of::<u32>() * 1000 * 1000,
            )
        };

        for x in 0..1000 * 1000 {
            assert_eq!(x as u32, arr[x]);
        }

        drop(o);

        core.shutdown();
    }

    #[test]
    fn with_header() {
        let config = UfoCoreConfig {
            writeback_temp_path: "/tmp",
            high_watermark: 1024 * 1024 * 1024,
            low_watermark: 512 * 1024 * 1024,
        };
        let core = UfoCore::new_ufo_core(config).expect("error getting core");

        let obj_cfg = UfoObjectConfig::new_config(
            1,
            1000 * 1000,
            size_of::<u32>(),
            Some(4096),
            Box::new(|start, end, fill| {
                let slice = unsafe {
                    std::slice::from_raw_parts_mut(fill.cast(), size_of::<u32>() * (end - start))
                };
                for idx in start..end {
                    slice[idx - start] = idx as u32;
                }
            }),
        );

        let o = core.allocate_ufo(obj_cfg);

        unsafe { assert_eq!(*o.header_ptr().cast::<u32>(), 0) };

        let arr = unsafe {
            std::slice::from_raw_parts_mut(
                o.body_ptr().cast::<u32>(),
                size_of::<u32>() * 1000 * 1000,
            )
        };

        for x in 0..1000 * 1000 {
            assert_eq!(x as u32, arr[x]);
        }

        drop(o);

        core.shutdown();
    }
}
