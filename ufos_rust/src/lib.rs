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

//TODO: wrapper instead of sending out an Arc
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

    pub fn reset_ufo(&self, ufo: &UfoHandle) {
        let wait_group = crossbeam::sync::WaitGroup::new();

        self.core
            .msg_send
            .send(UfoInstanceMsg::Reset(wait_group.clone(), ufo.id))
            .expect("Messages pipe broken");

        wait_group.wait();
    }

    pub fn free_ufo(&self, ufo: UfoHandle) {
        let wait_group = crossbeam::sync::WaitGroup::new();

        self.core
            .msg_send
            .send(UfoInstanceMsg::Free(wait_group.clone(), ufo.id))
            .expect("Messages pipe broken");

        wait_group.wait();
    }

    pub fn shutdown(self) {
        self.core.shutdown();
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
