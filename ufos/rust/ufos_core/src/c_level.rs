// use std::sync::MutexGuard;

// use anyhow::Result;
// use crossbeam::sync::WaitGroup;

// use crate::{UfoCore, UfoInstanceMsg, UfoCoreConfig, UfoObjectConfigPrototype, ufo_objects::UfoId, ufo_objects::{UfoObject, UfoPopulateError}};

// macro_rules! opaque_c_type {
//     ($wrapper_name:ident, $wrapped_type:ty) => {
//         impl $wrapper_name {
//             fn wrap(t: $wrapped_type) -> Self {
//                 $wrapper_name {
//                     ptr: Box::into_raw(Box::new(t)).cast(),
//                 }
//             }

//             fn none() -> Self {
//                 $wrapper_name {
//                     ptr: std::ptr::null_mut(),
//                 }
//             }

//             fn deref(&self) -> Option<&$wrapped_type> {
//                 if self.ptr.is_null() {
//                     None
//                 } else {
//                     Some(unsafe { &*self.ptr.cast() })
//                 }
//             }

//             #[allow(dead_code)]
//             fn deref_mut(&self) -> Option<&mut $wrapped_type> {
//                 if self.ptr.is_null() {
//                     None
//                 } else {
//                     Some(unsafe { &mut *self.ptr.cast() })
//                 }
//             }
//         }
//     };

//     ($wrapper_name:ident, $wrapped_type:ty, $free_name:ident) => {
//         opaque_c_type!($wrapper_name, $wrapped_type);

//         impl $wrapper_name {
//             #[no_mangle]
//             pub extern "C" fn $free_name(self) {}
//         }

//         impl Drop for $wrapper_name {
//             fn drop(&mut self) {
//                 if !self.ptr.is_null() {
//                     let mut the_ptr = std::ptr::null_mut();
//                     unsafe {
//                         std::ptr::swap(&mut the_ptr, &mut self.ptr);
//                         Box::<$wrapped_type>::from_raw(the_ptr.cast());
//                     }
//                 }
//             }
//         }
//     };
// }

// #[repr(C)]
// pub struct UfoInstance {
//     ptr: *mut libc::c_void,
// }
// opaque_c_type!(UfoInstance, UfoCore, free_ufo_instance);


// impl UfoInstance {
//     #[no_mangle]
//     pub unsafe extern "C" fn new_ufo_core(
//         writeback_temp_path: *const libc::c_char,
//         low_water_mark: libc::size_t,
//         high_water_mark: libc::size_t,
//     ) -> Self {
//         std::panic::catch_unwind(|| {
//             let wb = std::ffi::CStr::from_ptr(writeback_temp_path)
//                 .to_str().expect("invalid string")
//                 .to_string();
//             assert!(low_water_mark < high_water_mark);
//             let config = UfoCoreConfig {
//                 writeback_temp_path: wb,
//                 low_watermark: low_water_mark,
//                 high_watermark: high_water_mark,
//             };

//             let core = UfoCore::new_ufo_core(config);
//             match core {
//                 Err(_) => Self::none(),
//                 Ok(core) => Self::wrap(core),
//             }
//         })
//         .unwrap_or_else(|_| Self::none())
//     }

//     #[no_mangle]
//     pub extern "C" fn shutdown(self) {}

//     #[no_mangle]
//     pub extern "C" fn is_valid(&self) -> bool {
//         self.deref().is_some()
//     }

//     #[no_mangle]
//     pub extern "C" fn get_ufo_by_address(&self, ptr: usize) -> UfoObj{
//         std::panic::catch_unwind(|| {
//             self.deref()
//             .and_then( |core| {
//                 let id = core.core
//                     .get_ufo_by_address(ptr).ok()?
//                     .lock().ok()?
//                     .id;
//                 Some(UfoObj{ufo_id: id})
//             })
//             .unwrap_or_else(UfoObj::sentinel)
//         }).unwrap_or_else(|_| UfoObj::sentinel())
//     }

//     #[no_mangle]
//     pub extern "C" fn new_ufo(
//         &self,
//         prototype: &UfoPrototype,
//         ct: libc::size_t,
//         callback_data: *mut libc::c_void,
//         populate: extern "C" fn(*mut libc::c_void, libc::size_t, libc::size_t, *mut libc::c_uchar) -> i32,
//     ) -> UfoObj {
//         std::panic::catch_unwind(|| {
//             let callback_data_int = callback_data as usize;
//             let populate = move |start, end, to_populate| {
//                 let ret = populate(
//                     callback_data_int as *mut libc::c_void,
//                     start,
//                     end,
//                     to_populate,
//                 );

//                 if ret != 0 {
//                     Err(UfoPopulateError)
//                 }else{
//                     Ok(())
//                 }
//             };
//             let r = self
//                 .deref()
//                 .zip(prototype.deref())
//                 .map(move |(core, prototype)| {
//                     core.allocate_ufo_raw(prototype.new_config(ct, Box::new(populate)))
//                 });
//             match r {
//                 Some(Ok(ufo_id)) => UfoObj { ufo_id },
//                 _ => UfoObj::sentinel(),
//             }
//         })
//         .unwrap_or_else(|_| UfoObj::sentinel())
//     }
// }

// #[repr(C)]
// pub struct UfoPrototype {
//     ptr: *mut libc::c_void,
// }
// opaque_c_type!(UfoPrototype, UfoObjectConfigPrototype, free_ufo_prototype);


// impl UfoPrototype {
//     #[no_mangle]
//     pub extern "C" fn new_ufo_prototype(
//         header_size: libc::size_t,
//         stride: libc::size_t,
//         min_load_ct: libc::size_t,
//     ) -> UfoPrototype {
//         std::panic::catch_unwind(|| {
//             let min_load_ct = Some(min_load_ct).filter(|x| *x > 0);
//             Self::wrap(UfoObjectConfigPrototype::new_prototype(
//                 header_size,
//                 stride,
//                 min_load_ct,
//             ))
//         })
//         .unwrap_or_else(|_| Self::none())
//     }
// }

// #[repr(C)]
// pub struct UfoObj {
//     ptr: *mut libc::c_void,
// }

// impl UfoObj {
//     fn sentinel() -> UfoObj{
//         UfoObj { ufo_id: UfoId::sentinel() }
//     }

//     fn with_ufo<T, E, F>(core: &UfoInstance, this: &Self, f: F) -> Option<T>
//     where
//         F: FnOnce(MutexGuard<UfoObject>) -> Result<T, E>
//     {
//         Some(this.ufo_id)
//             .filter(|id| !id.is_sentinel())
//             .zip(core.deref())
//             .and_then(|(id, core)| {
//                 core.core
//                     .get_ufo_by_id(id).ok()?
//                     .lock().ok()
//                     .map(f)?
//                     .ok()
//             })
//     }

//     #[no_mangle]
//     pub unsafe extern "C" fn reset(core: &UfoInstance, this: &Self) -> i32 {
//         std::panic::catch_unwind(|| {
//             Self::with_ufo(core, this, |ufo| ufo.reset())
//                 .map(|()| 0)
//                 .unwrap_or(-1)
//         })
//         .unwrap_or(-1)
//     }

//     #[no_mangle]
//     pub extern "C" fn header_ptr(core: &UfoInstance, this: &Self) -> *mut std::ffi::c_void {
//         std::panic::catch_unwind(|| {
//             Self::with_ufo(core, this, |ufo| { unsafe {
//                 let header_offset = ufo.config.header_size_with_padding - ufo.config.header_size;
//                 Ok::<*mut std::ffi::c_void, ()>(ufo.mmap
//                     .as_ptr()
//                     .add(header_offset)
//                     .cast())
//             }}).unwrap_or(std::ptr::null_mut())
//         })
//         .unwrap_or_else(|_| std::ptr::null_mut())
//     }

//     #[no_mangle]
//     pub extern "C" fn body_ptr(core: &UfoInstance, this: &Self) -> *mut std::ffi::c_void {
//         std::panic::catch_unwind(|| {
//             Self::with_ufo(core, this, |ufo| { unsafe {
//                 Ok::<*mut std::ffi::c_void, ()>(ufo.mmap
//                     .as_ptr()
//                     .add(ufo.config.header_size_with_padding)
//                     .cast())
//             }}).unwrap_or(std::ptr::null_mut())
//         })
//         .unwrap_or(std::ptr::null_mut())
//     }

//     #[no_mangle]
//     pub extern "C" fn free_ufo(core: &UfoInstance, ufo: UfoObj) {
//         std::panic::catch_unwind(|| {
//             if let Some(core) = core.deref() {
//                 let waiter = WaitGroup::new();
//                 let r = core.core.msg_send.send(UfoInstanceMsg::Free(waiter.clone(), ufo.ufo_id));
//                 if let Ok(()) = r {
//                     waiter.wait();
//                 }
//             }
//         }).unwrap_or(())
//     }
// }

// #[no_mangle]
// pub extern "C" fn begin_ufo_log() {
//     stderrlog::new()
//         // .module("ufo_core")
//         .verbosity(4)
//         .timestamp(stderrlog::Timestamp::Millisecond)
//         .init()
//         .unwrap();
// }