use std::sync::{Arc, RwLockWriteGuard};

use anyhow::Result;

use libc::c_void;
use ufos_core::{UfoCoreConfig, UfoObjectConfigPrototype, UfoPopulateError, WrappedUfoObject, UfoObject};



macro_rules! opaque_c_type {
    ($wrapper_name:ident, $wrapped_type:ty) => {
        impl $wrapper_name {
            fn wrap(t: $wrapped_type) -> Self {
                $wrapper_name {
                    ptr: Box::into_raw(Box::new(t)).cast(),
                }
            }

            fn none() -> Self {
                $wrapper_name {
                    ptr: std::ptr::null_mut(),
                }
            }

            fn deref(&self) -> Option<&$wrapped_type> {
                if self.ptr.is_null() {
                    None
                } else {
                    Some(unsafe { &*self.ptr.cast() })
                }
            }

            #[allow(dead_code)]
            fn deref_mut(&self) -> Option<&mut $wrapped_type> {
                if self.ptr.is_null() {
                    None
                } else {
                    Some(unsafe { &mut *self.ptr.cast() })
                }
            }
        }

        impl Drop for $wrapper_name {
            fn drop(&mut self) {
                if !self.ptr.is_null() {
                    let mut the_ptr = std::ptr::null_mut();
                    unsafe {
                        std::ptr::swap(&mut the_ptr, &mut self.ptr);
                        assert!(!the_ptr.is_null());
                        Box::<$wrapped_type>::from_raw(the_ptr.cast());
                    }
                }
            }
        }
    };
}

#[repr(C)]
pub struct UfoCore {
    ptr: *mut c_void,
}
opaque_c_type!(UfoCore, Arc<ufos_core::UfoCore>);

type UfoPopulateData = *mut c_void;
type UfoPopulateCallout = extern "C" fn(UfoPopulateData, libc::size_t, libc::size_t, *mut libc::c_uchar) -> i32;

impl UfoCore {
    #[no_mangle]
    pub unsafe extern "C" fn ufo_new_core(
        writeback_temp_path: *const libc::c_char,
        low_water_mark: libc::size_t,
        high_water_mark: libc::size_t,
    ) -> Self {
        std::panic::catch_unwind(|| {
            let wb = std::ffi::CStr::from_ptr(writeback_temp_path)
                .to_str().expect("invalid string")
                .to_string();
            
            let mut low_water_mark = low_water_mark;
            let mut high_water_mark = high_water_mark;

            if low_water_mark > high_water_mark {
                std::mem::swap(&mut low_water_mark, &mut high_water_mark);
            }
            assert!(low_water_mark < high_water_mark);

            let config = UfoCoreConfig {
                writeback_temp_path: wb,
                low_watermark: low_water_mark,
                high_watermark: high_water_mark,
            };

            let core = ufos_core::UfoCore::new(config);
            match core {
                Err(_) => Self::none(),
                Ok(core) => Self::wrap(core),
            }
        })
        .unwrap_or_else(|_| Self::none())
    }

    #[no_mangle]
    pub extern "C" fn ufo_core_shutdown(self) {}

    #[no_mangle]
    pub extern "C" fn ufo_core_is_error(&self) -> bool {
        self.deref().is_none()
    }

    #[no_mangle]
    pub extern "C" fn ufo_get_by_address(&self, ptr: *mut libc::c_void) -> UfoObj{
        std::panic::catch_unwind(|| {
            self.deref()
            .and_then( |core| {
                let ufo = core
                    .get_ufo_by_address(ptr as usize).ok()?;
                Some(UfoObj::wrap(ufo))
            })
            .unwrap_or_else(UfoObj::none)
        }).unwrap_or_else(|_| UfoObj::none())
    }

    #[no_mangle]
    pub extern "C" fn ufo_address_is_ufo_object(&self, ptr: *mut libc::c_void) -> bool{
        std::panic::catch_unwind(|| {
            self.deref()
            .and_then( |core| {
                core.get_ufo_by_address(ptr as usize).ok()?;
                Some(true)
            })
            .unwrap_or(false)
        }).unwrap_or(false)
    }

    #[no_mangle]
    pub extern "C" fn ufo_new_no_prototype(
        &self,
        header_size: libc::size_t,
        stride: libc::size_t,
        min_load_ct: libc::size_t,
        read_only: bool,
        ct: libc::size_t,
        callback_data: UfoPopulateData,
        populate: UfoPopulateCallout,
    ) -> UfoObj {
        std::panic::catch_unwind(|| {
            let min_load_ct = Some(min_load_ct).filter(|x| *x > 0);
            let prototype = UfoObjectConfigPrototype::new_prototype(
                header_size,
                stride,
                min_load_ct,
                read_only,
            );

        let callback_data_int = callback_data as usize;
            let populate = move |start, end, to_populate| {
                let ret = populate(
                    callback_data_int as *mut c_void,
                    start,
                    end,
                    to_populate,
                );

                if ret != 0 {
                    Err(UfoPopulateError)
                }else{
                    Ok(())
                }
            };
            let r = self
                .deref()
                .map(move |core| {
                    core.allocate_ufo(prototype.new_config(ct, Box::new(populate)))
                });
            match r {
                Some(Ok(ufo)) => UfoObj::wrap(ufo),
                _ => UfoObj::none(),
            }
        })
        .unwrap_or_else(|_| UfoObj::none())
    }


    #[no_mangle]
    pub extern "C" fn ufo_new_with_prototype(
        &self,
        prototype: &UfoPrototype,
        ct: libc::size_t,
        callback_data: *mut c_void,
        populate: extern "C" fn(*mut c_void, libc::size_t, libc::size_t, *mut libc::c_uchar) -> i32,
    ) -> UfoObj {
        std::panic::catch_unwind(|| {
            let callback_data_int = callback_data as usize;
            let populate = move |start, end, to_populate| {
                let ret = populate(
                    callback_data_int as *mut c_void,
                    start,
                    end,
                    to_populate,
                );

                if ret != 0 {
                    Err(UfoPopulateError)
                }else{
                    Ok(())
                }
            };
            let r = self
                .deref()
                .zip(prototype.deref())
                .map(move |(core, prototype)| {
                    core.allocate_ufo(prototype.new_config(ct, Box::new(populate)))
                });
            match r {
                Some(Ok(ufo)) => UfoObj::wrap(ufo),
                _ => UfoObj::none(),
            }
        })
        .unwrap_or_else(|_| UfoObj::none())
    }
}

#[repr(C)]
pub struct UfoPrototype {
    ptr: *mut c_void,
}
opaque_c_type!(UfoPrototype, UfoObjectConfigPrototype);


impl UfoPrototype {
    #[no_mangle]
    pub extern "C" fn ufo_new_prototype(
        header_size: libc::size_t,
        stride: libc::size_t,
        min_load_ct: libc::size_t,
        read_only: bool,
    ) -> UfoPrototype {
        std::panic::catch_unwind(|| {
            let min_load_ct = Some(min_load_ct).filter(|x| *x > 0);
            Self::wrap(UfoObjectConfigPrototype::new_prototype(
                header_size,
                stride,
                min_load_ct,
                read_only,
            ))
        })
        .unwrap_or_else(|_| Self::none())
    }

    #[no_mangle]
    pub extern "C" fn ufo_prototype_is_error(&self) -> bool {
        self.deref().is_none()
    }

    #[no_mangle]
    pub extern "C" fn ufo_free_prototype(self){}
}

#[repr(C)]
pub struct UfoObj {
    ptr: *mut c_void,
}

opaque_c_type!(UfoObj, WrappedUfoObject);

impl UfoObj {
    fn with_ufo<F, T, E>(&self, f: F) -> Option<T>
    where
        F: FnOnce(RwLockWriteGuard<UfoObject>) -> Result<T, E>
    {
        self.deref()
            .and_then(|ufo| {
                    ufo.write().ok()
                    .map(f)?.ok()
            })
    }

    #[no_mangle]
    pub unsafe extern "C" fn ufo_reset(&self) -> i32 {
        std::panic::catch_unwind(|| {
            self.with_ufo(|ufo| ufo.reset())
                .map(|w| w.wait())
                .map(|()| 0)
                .unwrap_or(-1)
        })
        .unwrap_or(-1)
    }

    #[no_mangle]
    pub extern "C" fn ufo_header_ptr(&self) -> *mut std::ffi::c_void {
        std::panic::catch_unwind(|| {
            self.with_ufo(|ufo| Ok::<*mut c_void, ()>(ufo.header_ptr()))
            .unwrap_or_else(|| std::ptr::null_mut())
        })
        .unwrap_or_else(|_| std::ptr::null_mut())
    }

    #[no_mangle]
    pub extern "C" fn ufo_body_ptr(&self) -> *mut std::ffi::c_void {
        std::panic::catch_unwind(|| {
            self.with_ufo(|ufo| Ok::<*mut c_void, ()>(ufo.body_ptr()))
            .unwrap_or_else(|| std::ptr::null_mut())
        })
        .unwrap_or_else(|_| std::ptr::null_mut())
    }

    #[no_mangle]
    pub extern "C" fn ufo_free(self) {
        std::panic::catch_unwind(|| {
            self.deref()
            .and_then(|ufo| ufo.write().ok()?.free().ok())
            .map(|w| w.wait())
            .unwrap_or(())
        }).unwrap_or(())
    }

    #[no_mangle]
    pub extern "C" fn ufo_is_error(&self) -> bool {
        self.deref().is_none()
    }
}

#[no_mangle]
pub extern "C" fn ufo_begin_log() {
    stderrlog::new()
        // .module("ufo_core")
        .verbosity(4)
        .timestamp(stderrlog::Timestamp::Millisecond)
        .init()
        .unwrap();
}