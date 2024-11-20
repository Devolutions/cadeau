use xmf_sys::{
    XmfVpxDecoder, XmfVpxDecoderConfig, XmfVpxDecoder_Create, XmfVpxDecoder_Decode, XmfVpxDecoder_Destroy,
    XmfVpxDecoder_GetLastError, XmfVpxDecoder_GetNextFrame,
};

use super::{VpxCodec, VpxError, VpxImage};

pub struct VpxDecoderBuilder {
    threads: Option<u32>,
    w: Option<u32>, // Width (set to 0 if unknown)
    h: Option<u32>, // Height (set to 0 if unknown)
    codec: Option<VpxCodec>,
}

pub struct VpxDecoder {
    ptr: *mut XmfVpxDecoder,
}

impl VpxDecoder {
    pub fn builder() -> VpxDecoderBuilder {
        VpxDecoderBuilder::new()
    }

    pub fn decode(&mut self, data: &[u8]) -> Result<(), VpxError> {
        // SAFETY: FFI call with no outstanding precondition.
        let ret = unsafe {
            XmfVpxDecoder_Decode(
                self.ptr,
                data.as_ptr(),
                u32::try_from(data.len()).map_err(|_| "data.len cannot be converted to u32")?,
            )
        };
        if ret == 0 {
            Ok(())
        } else {
            Err(self.last_error())
        }
    }

    pub fn next_frame(&mut self) -> Result<VpxImage<'_>, VpxError> {
        // SAFETY: FFI call with no outstanding precondition.
        let image = unsafe { XmfVpxDecoder_GetNextFrame(self.ptr) };
        if !image.is_null() {
            // SAFETY: Safe to call, the pointer is garanteed to be valid until the next call to the decoder.
            // and since the VpxImage have a lifetime tied to the VpxDecoder, it is safe to create and use it.
            Ok(unsafe { VpxImage::from_raw(image) })
        } else {
            Err(self.last_error())
        }
    }

    fn last_error(&self) -> VpxError {
        // SAFETY: Always safe to call, even if no error or no pointer.
        let error = unsafe { XmfVpxDecoder_GetLastError(self.ptr) };
        error.into()
    }
}

impl Drop for VpxDecoder {
    fn drop(&mut self) {
        // SAFETY: Safe to call, as the VpxDecoder owns the pointer.
        unsafe {
            XmfVpxDecoder_Destroy(self.ptr);
        };
    }
}

impl Default for VpxDecoderBuilder {
    fn default() -> Self {
        Self::new()
    }
}

impl VpxDecoderBuilder {
    pub fn new() -> Self {
        Self {
            threads: None,
            w: None,
            h: None,
            codec: None,
        }
    }

    #[must_use]
    pub fn threads(mut self, threads: u32) -> Self {
        self.threads = Some(threads);
        self
    }

    #[must_use]
    pub fn width(mut self, w: u32) -> Self {
        self.w = Some(w);
        self
    }

    #[must_use]
    pub fn height(mut self, h: u32) -> Self {
        self.h = Some(h);
        self
    }

    #[must_use]
    pub fn codec(mut self, codec: VpxCodec) -> Self {
        self.codec = Some(codec);
        self
    }

    pub fn build(self) -> Result<VpxDecoder, VpxError> {
        let config = XmfVpxDecoderConfig {
            threads: self.threads.unwrap_or(0),
            w: self.w.unwrap_or(0),
            h: self.h.unwrap_or(0),
            codec: self.codec.unwrap_or(VpxCodec::VP8).into(),
        };

        // SAFETY: FFI call with no outstanding precondition.
        let ptr = unsafe { XmfVpxDecoder_Create(config) };

        if ptr.is_null() {
            return Err(VpxError::NullPointer);
        }

        Ok(VpxDecoder { ptr })
    }
}
