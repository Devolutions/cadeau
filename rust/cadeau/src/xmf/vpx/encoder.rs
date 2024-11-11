use super::{VpxCodec, VpxImage};

use xmf_sys::{
    vpx::XmfVpxEncoderError, XmfVpxEncoder, XmfVpxEncoder_Create, XmfVpxEncoder_Destroy, XmfVpxEncoder_EncodeFrame,
    XmfVpxEncoder_Flush, XmfVpxEncoder_FreeEncodedFrame, XmfVpxEncoder_GetEncodedFrame, XmfVpxEncoder_GetLastError,
};

pub struct VpxEncoderConfig(xmf_sys::vpx::XmfVpxEncoderConfig);

pub struct VpxEncoderConfigBuilder {
    codec: Option<VpxCodec>,
    width: Option<u32>,
    height: Option<u32>,
    bitrate: Option<u32>,
    timebase_num: Option<u32>,
    timebase_den: Option<u32>,
    threads: Option<u32>,
}

pub struct VpxEncoder {
    ptr: *mut XmfVpxEncoder,
}

impl VpxEncoder {
    pub fn new(config: VpxEncoderConfig) -> Self {
        let ptr = unsafe { XmfVpxEncoder_Create(config.0) };
        Self { ptr }
    }

    ///@param pts      Presentation timestamp of the frame.
    ///@param flags    Flags for encoding (e.g., keyframe).
    pub fn encode_frame(
        &mut self,
        image: &VpxImage,
        pts: i64,
        duration: i64,
        flags: u32,
    ) -> Result<(), XmfVpxEncoderError> {
        let ret = unsafe { XmfVpxEncoder_EncodeFrame(self.ptr, image.ptr, pts, duration, flags) };
        if ret != 0 {
            let error = unsafe { XmfVpxEncoder_GetLastError(self.ptr) };
            return Err(error);
        }

        Ok(())
    }

    pub fn next_frame(&mut self) -> Result<Option<Vec<u8>>, XmfVpxEncoderError> {
        let mut output: *mut u8 = std::ptr::null_mut();
        let mut output_size: usize = 0;
        let ret = unsafe { XmfVpxEncoder_GetEncodedFrame(self.ptr, &mut output, &mut output_size) };
        if ret == 0 {
            let mut vec = Vec::with_capacity(output_size);

            if output.is_null() {
                return Ok(None);
            }

            unsafe {
                vec.set_len(output_size);
                std::ptr::copy_nonoverlapping(output, vec.as_mut_ptr(), output_size);
            }

            unsafe {
                XmfVpxEncoder_FreeEncodedFrame(output);
            }

            Ok(Some(vec))
        } else {
            let error = unsafe { XmfVpxEncoder_GetLastError(self.ptr) };
            Err(error)
        }
    }

    pub fn flush(&mut self) -> Result<(), XmfVpxEncoderError> {
        let ret = unsafe { XmfVpxEncoder_Flush(self.ptr) };
        if ret == 0 {
            Ok(())
        } else {
            let error = unsafe { XmfVpxEncoder_GetLastError(self.ptr) };
            Err(error)
        }
    }
}

impl Drop for VpxEncoder {
    fn drop(&mut self) {
        unsafe {
            XmfVpxEncoder_Destroy(self.ptr);
        };
    }
}

impl VpxEncoderConfigBuilder {
    pub fn new() -> Self {
        Self {
            codec: None,
            width: None,
            height: None,
            bitrate: None,
            timebase_num: None,
            timebase_den: None,
            threads: None,
        }
    }

    #[must_use]
    pub fn codec(mut self, codec: VpxCodec) -> Self {
        self.codec = Some(codec);
        self
    }

    #[must_use]
    pub fn width(mut self, width: u32) -> Self {
        self.width = Some(width);
        self
    }

    #[must_use]
    pub fn height(mut self, height: u32) -> Self {
        self.height = Some(height);
        self
    }

    #[must_use]
    pub fn bitrate(mut self, bitrate: u32) -> Self {
        self.bitrate = Some(bitrate);
        self
    }

    #[must_use]
    pub fn timebase_num(mut self, timebase_num: u32) -> Self {
        self.timebase_num = Some(timebase_num);
        self
    }

    #[must_use]
    pub fn timebase_den(mut self, timebase_den: u32) -> Self {
        self.timebase_den = Some(timebase_den);
        self
    }

    #[must_use]
    pub fn threads(mut self, threads: u32) -> Self {
        self.threads = Some(threads);
        self
    }

    pub fn build(self) -> VpxEncoderConfig {
        let config = xmf_sys::vpx::XmfVpxEncoderConfig {
            codec: self.codec.unwrap_or(VpxCodec::VP8).into(),
            width: self.width.unwrap_or(0),
            height: self.height.unwrap_or(0),
            bitrate: self.bitrate.unwrap_or(0),
            timebase_num: self.timebase_num.unwrap_or(0),
            timebase_den: self.timebase_den.unwrap_or(0),
            threads: self.threads.unwrap_or(0),
        };
        VpxEncoderConfig(config)
    }
}

impl VpxEncoderConfig {
    pub fn builder() -> VpxEncoderConfigBuilder {
        VpxEncoderConfigBuilder::new()
    }
}
