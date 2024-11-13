use super::{VpxCodec, VpxError, VpxImage, VpxPacket};

use xmf_sys::{
    vpx::XmfVpxEncoderError, XmfVpxEncoder, XmfVpxEncoder_Create, XmfVpxEncoder_Destroy, XmfVpxEncoder_EncodeFrame,
    XmfVpxEncoder_Flush, XmfVpxEncoder_FreeEncodedFrame, XmfVpxEncoder_GetEncodedFrame, XmfVpxEncoder_GetLastError,
    XmfVpxEncoder_GetPacket,
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
        // Safety: The way we build the config ensures that it is always valid.
        let ptr = unsafe { XmfVpxEncoder_Create(config.0) };
        Self { ptr }
    }

    pub fn encode_frame(&mut self, image: &VpxImage, pts: i64, duration: usize, flags: u32) -> Result<(), VpxError> {
        // Safety: Always safe to call, even if the pointer is null.
        let ret = unsafe { XmfVpxEncoder_EncodeFrame(self.ptr, image.ptr, pts, duration, flags) };
        if ret != 0 {
            // Safety: Always safe to call, even if the pointer is null.
            let error = unsafe { XmfVpxEncoder_GetLastError(self.ptr) };
            return Err(error.into());
        }

        Ok(())
    }

    pub fn next_frame(&mut self) -> Result<Option<Vec<u8>>, VpxError> {
        let mut output: *mut u8 = std::ptr::null_mut();
        let mut output_size: usize = 0;

        // Safety: Always safe to call, even if the pointer is null.
        let ret = unsafe { XmfVpxEncoder_GetEncodedFrame(self.ptr, &mut output, &mut output_size) };
        if ret == 0 {
            if output.is_null() {
                return Ok(None);
            }
            let mut vec = vec![0u8; output_size];

            // Safety: Safe to call since we have allocated the vector with the correct size.
            unsafe {
                std::ptr::copy_nonoverlapping(output, vec.as_mut_ptr(), output_size);
            }

            // Safety: safe to call as we immediately copy the data to a Vec.
            unsafe {
                XmfVpxEncoder_FreeEncodedFrame(output);
            }

            Ok(Some(vec))
        } else {
            // Safety: Always safe to call, even if the pointer is null.
            let error = unsafe { XmfVpxEncoder_GetLastError(self.ptr) };
            Err(error.into())
        }
    }

    /// # Safety
    ///
    /// The caller must make sure to use the packets before calling any other function on the encoder.
    pub unsafe fn packet_iterator(&mut self) -> PacketIterators<'_> {
        PacketIterators::new(self)
    }

    pub fn flush(&mut self) -> Result<(), XmfVpxEncoderError> {
        // Safety: This method never panics, even if the pointer is null.
        let ret = unsafe { XmfVpxEncoder_Flush(self.ptr) };
        if ret == 0 {
            Ok(())
        } else {
            // Safety: Always safe to call, even if the pointer is null.
            let error = unsafe { XmfVpxEncoder_GetLastError(self.ptr) };
            Err(error)
        }
    }
}

type VpxCodecIter = *const std::ffi::c_void;
pub struct PacketIterators<'a> {
    iter: VpxCodecIter,
    encoder: &'a VpxEncoder,
}

impl<'a> PacketIterators<'a> {
    fn new(encoder: &'a VpxEncoder) -> Self {
        let iter = std::ptr::null_mut();
        Self { iter, encoder }
    }
}

impl Iterator for PacketIterators<'_> {
    type Item = VpxPacket;

    fn next(&mut self) -> Option<Self::Item> {
        // Safety: Should be safe to call as the encoder pointer is never exposed to the caller.
        let ptr = unsafe { XmfVpxEncoder_GetPacket(self.encoder.ptr, &mut self.iter as *mut VpxCodecIter) };

        if ptr.is_null() {
            return None;
        }

        let packet = VpxPacket { ptr };

        // Safety: use packet before any other call to VpxEncoder
        unsafe {
            if packet.is_empty() {
                return None;
            }
        }

        Some(packet)
    }
}

impl Drop for VpxEncoder {
    fn drop(&mut self) {
        // Safety: Safe to call as the pointer is never exposed to the caller.
        unsafe {
            XmfVpxEncoder_Destroy(self.ptr);
        };
    }
}

impl Default for VpxEncoderConfigBuilder {
    fn default() -> Self {
        Self::new()
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