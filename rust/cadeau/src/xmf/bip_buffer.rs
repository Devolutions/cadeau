use xmf_sys::{
    XmfBipBuffer, XmfBipBuffer_BufferSize, XmfBipBuffer_Clear, XmfBipBuffer_Free, XmfBipBuffer_Grow, XmfBipBuffer_New,
    XmfBipBuffer_Read, XmfBipBuffer_UsedSize, XmfBipBuffer_Write,
};

pub struct BipBuffer {
    ptr: *mut XmfBipBuffer,
}

impl BipBuffer {
    #[inline]
    pub fn new(size: usize) -> Self {
        // SAFETY: FFI call with no outstanding precondition.
        let inner = unsafe { XmfBipBuffer_New(size) };

        Self { ptr: inner }
    }

    #[inline]
    pub fn grow(&mut self, size: usize) -> bool {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfBipBuffer_Grow(self.ptr, size) }
    }

    #[inline]
    pub fn clear(&mut self) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfBipBuffer_Clear(self.ptr) }
    }

    #[inline]
    pub fn used_size(&mut self) -> usize {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfBipBuffer_UsedSize(self.ptr) }
    }

    #[inline]
    pub fn buffer_size(&mut self) -> usize {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfBipBuffer_BufferSize(self.ptr) }
    }

    #[inline]
    pub fn read(&mut self, data: &mut [u8]) -> usize {
        // SAFETY: We are ensuring that size == data.size().
        let n = unsafe { XmfBipBuffer_Read(self.ptr, data.as_mut_ptr(), data.len()) };
        usize::try_from(n).expect("n is never negative and is logically used as a pointer-sized type")
    }

    #[inline]
    pub fn write(&mut self, data: &[u8]) -> usize {
        // SAFETY: We are ensuring that size == data.size().
        let n = unsafe { XmfBipBuffer_Write(self.ptr, data.as_ptr(), data.len()) };
        usize::try_from(n).expect("n is never negative and is logically used as a pointer-sized type")
    }

    /// Returns a raw pointer to the underlying FFI handle.
    #[inline]
    pub const fn as_ptr(&self) -> *mut XmfBipBuffer {
        self.ptr
    }
}

impl Drop for BipBuffer {
    #[inline]
    fn drop(&mut self) {
        // SAFETY: The pointer is owned.
        unsafe { XmfBipBuffer_Free(self.ptr) };
    }
}
