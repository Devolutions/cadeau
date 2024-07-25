use core::fmt;
use std::ffi::CString;
use std::mem::MaybeUninit;
use std::path::Path;

use cadeau_sys::xmf::{XmfImage_FreeData, XmfImage_LoadFile};

#[derive(Debug, Clone)]
pub enum ImageError {
    BadArgument { name: &'static str },
    Generic,
}

impl std::error::Error for ImageError {}

impl fmt::Display for ImageError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ImageError::BadArgument { name } => write!(f, "bad argument: {name}"),
            ImageError::Generic => write!(f, "generic failure"),
        }
    }
}

pub struct Image {
    data: *mut u8,
    width: usize,
    height: usize,
    step: usize,
}

impl Image {
    pub fn load_file(path: &Path) -> Result<Self, ImageError> {
        let path = path
            .to_str()
            .and_then(|s| CString::new(s).ok())
            .ok_or(ImageError::BadArgument { name: "path" })?;

        let mut data = MaybeUninit::zeroed();
        let mut width = MaybeUninit::zeroed();
        let mut height = MaybeUninit::zeroed();
        let mut step = MaybeUninit::zeroed();

        // SAFETY: FFI call with no outstanding precondition.
        let ret = unsafe {
            XmfImage_LoadFile(
                path.as_ptr(),
                data.as_mut_ptr(),
                width.as_mut_ptr(),
                height.as_mut_ptr(),
                step.as_mut_ptr(),
            )
        };

        if !ret {
            return Err(ImageError::Generic);
        }

        // SAFETY: XmfImage_LoadFile returned success status, and is supposed to initialize the values.
        let data = unsafe { data.assume_init() };

        // SAFETY: XmfImage_LoadFile returned success status, and is supposed to initialize the values.
        let width = unsafe { width.assume_init() };

        // SAFETY: XmfImage_LoadFile returned success status, and is supposed to initialize the values.
        let height = unsafe { height.assume_init() };

        // SAFETY: XmfImage_LoadFile returned success status, and is supposed to initialize the values.
        let step = unsafe { step.assume_init() };

        let width = usize::try_from(width).map_err(|_| ImageError::BadArgument { name: "width" })?;
        let height = usize::try_from(height).map_err(|_| ImageError::BadArgument { name: "height" })?;
        let step = usize::try_from(step).map_err(|_| ImageError::BadArgument { name: "step" })?;

        Ok(Self {
            data,
            width,
            height,
            step,
        })
    }

    #[inline]
    pub fn width(&self) -> usize {
        self.width
    }

    #[inline]
    pub fn height(&self) -> usize {
        self.height
    }

    #[inline]
    pub fn step(&self) -> usize {
        self.step
    }

    #[inline]
    pub fn data(&self) -> &[u8] {
        let len = self.height * self.step;

        // SAFETY: The buffer is expected to contain at least height * step elements.
        // FIXME: Truth is that the C library is not checking for this property, and we may segfault if the file loaded in memory is maliciously crafted.
        unsafe { core::slice::from_raw_parts(self.data, len) }
    }

    #[inline]
    pub fn data_mut(&mut self) -> &mut [u8] {
        let len = self.height * self.step;

        // SAFETY: The buffer is expected to contain at least height * step elements.
        // FIXME: Truth is that the C library is not checking for this property, and we may segfault if the file loaded in memory is maliciously crafted.
        unsafe { core::slice::from_raw_parts_mut(self.data, len) }
    }

    #[inline]
    /// Returns a raw pointer to the underlying FFI handle.
    pub const fn data_ptr(&self) -> *mut u8 {
        self.data
    }
}

impl Drop for Image {
    #[inline]
    fn drop(&mut self) {
        // SAFETY: The pointer is owned.
        unsafe { XmfImage_FreeData(self.data) };
    }
}
