#![feature(ptr_internals, once_cell, slice_ptr_get)]

pub mod mmap_wrapers;
pub mod ufo_instance;

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
