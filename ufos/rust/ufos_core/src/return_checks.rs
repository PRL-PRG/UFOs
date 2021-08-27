use std::io::Error;

use libc::c_void;

pub(crate) fn check_return_zero(result: i32) -> Result<(), Error> {
    match result {
        0 => Ok(()),
        -1 => Err(Error::last_os_error()),
        _ => panic!("return other than 0 or -1 ({})", result),
    }
}

pub(crate) fn check_return_nonneg(result: i32) -> Result<i32, Error> {
    match result {
        -1 => Err(Error::last_os_error()),
        x if x < -1 => panic!("return other than 0 or -1 ({})", result),
        x => Ok(x),
    }
}

pub(crate) fn check_ptr_nonneg(result: *mut c_void) -> Result<*mut c_void, Error> {
    match result as isize {
        -1 => Err(Error::last_os_error()),
        _ => Ok(result),
    }
}
