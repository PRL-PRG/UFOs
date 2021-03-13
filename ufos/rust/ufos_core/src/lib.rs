#![feature(ptr_internals, once_cell, slice_ptr_get)]

mod errors;
mod math;
mod mmap_wrapers;
mod return_checks;
mod ufo_core;
mod ufo_objects;

pub use errors::*;
pub use ufo_core::*;
pub use ufo_objects::*;