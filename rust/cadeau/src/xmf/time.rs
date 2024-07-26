use std::ptr::null_mut;

use xmf_sys::{XmfTimeSource, XmfTimeSource_Get, XmfTimeSource_System};

pub struct TimeSource {
    inner: XmfTimeSource,
}

impl TimeSource {
    #[inline]
    pub fn system() -> Self {
        Self {
            inner: XmfTimeSource {
                func: XmfTimeSource_System,
                param: null_mut(),
            },
        }
    }

    #[inline]
    pub fn get(&mut self) -> u64 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfTimeSource_Get(&mut self.inner) }
    }
}
