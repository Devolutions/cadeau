use xmf_sys::{
    XmfVpxDecoder_Decode, vpx::XmfVpxDecoderError, XmfVpxDecoder, XmfVpxDecoder_Create, XmfVpxDecoder_Destroy,
    XmfVpxDecoder_GetLastError, XmfVpxDecoder_GetNextFrame,
};

use super::{VpxCodec, VpxImage};

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
        let ptr = unsafe { XmfVpxDecoder_Create(config.0) };
        Self { ptr }
    }

    pub fn decode(&mut self, data: &[u8]) -> Result<(), XmfVpxDecoderError> {
        // SAFETY: FFI call with no outstanding precondition.
        let ret = unsafe { XmfVpxDecoder_Decode(self.ptr, data.as_ptr(), data.len() as u32) };
        if ret == 0 {
            Ok(())
        } else {
            let error = unsafe { XmfVpxDecoder_GetLastError(self.ptr) };
            Err(error)
        }
    }

    pub fn next_frame(&mut self) -> Result<VpxImage, XmfVpxDecoderError> {
        // SAFETY: FFI call with no outstanding precondition.
        let image = unsafe { XmfVpxDecoder_GetNextFrame(self.ptr) };
        if !image.is_null() {
            Ok(VpxImage { ptr: image })
        } else {
            let error = unsafe { XmfVpxDecoder_GetLastError(self.ptr) };
            Err(error)
        }
    }
}

impl Drop for VpxDecoder {
    fn drop(&mut self) {
        unsafe {
            XmfVpxDecoder_Destroy(self.ptr);
        };
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
    pub fn w(mut self, w: u32) -> Self {
        self.w = Some(w);
        self
    }

    #[must_use]
    pub fn h(mut self, h: u32) -> Self {
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