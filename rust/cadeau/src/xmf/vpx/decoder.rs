use xmf_sys::{
    vpx::XmfVpxDecoder, XmfVpxDecoder_Create, XmfVpxDecoder_Decode, XmfVpxDecoder_Destroy, XmfVpxDecoder_GetLastError,
    XmfVpxDecoder_GetNextFrame,
};

use super::{VpxCodec, VpxError, VpxImage};

pub struct VpxDecoderConfig(xmf_sys::vpx::XmfVpxDecoderConfig);

pub struct VpxDecoderConfigBuilder {
    threads: Option<u32>, // Corresponds to 'unsigned int' in C
    w: Option<u32>,       // Width (set to 0 if unknown)
    h: Option<u32>,       // Height (set to 0 if unknown)
    codec: Option<VpxCodec>,
}

pub struct VpxDecoder {
    ptr: *mut XmfVpxDecoder,
}

impl VpxDecoder {
    pub fn new(config: VpxDecoderConfig) -> Self {
        // SAFETY: Safe to call, as the way the config is built ensures that the fields are valid.
        let ptr = unsafe { XmfVpxDecoder_Create(config.0) };
        Self { ptr }
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
            // SAFETY: Always safe to call, even if the pointer is null.
            let error = unsafe { XmfVpxDecoder_GetLastError(self.ptr) };
            Err(error.into())
        }
    }

    pub fn next_frame(&mut self) -> Result<VpxImage, VpxError> {
        // SAFETY: FFI call with no outstanding precondition.
        let image = unsafe { XmfVpxDecoder_GetNextFrame(self.ptr) };
        if !image.is_null() {
            Ok(VpxImage { ptr: image })
        } else {
            // SAFETY: Always safe to call, even if the pointer is null.
            let error = unsafe { XmfVpxDecoder_GetLastError(self.ptr) };
            Err(error.into())
        }
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

impl Default for VpxDecoderConfigBuilder {
    fn default() -> Self {
        Self::new()
    }
}

impl VpxDecoderConfigBuilder {
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

    pub fn build(self) -> VpxDecoderConfig {
        let config = xmf_sys::vpx::XmfVpxDecoderConfig {
            threads: self.threads.unwrap_or(0),
            w: self.w.unwrap_or(0),
            h: self.h.unwrap_or(0),
            codec: self.codec.unwrap_or(VpxCodec::VP8).into(),
        };
        VpxDecoderConfig(config)
    }
}

impl VpxDecoderConfig {
    pub fn builder() -> VpxDecoderConfigBuilder {
        VpxDecoderConfigBuilder::new()
    }
}
