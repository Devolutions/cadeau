use xmf_sys::{
    XmfVpxEncoder, XmfVpxEncoderConfig, XmfVpxEncoderErrorCode, XmfVpxEncoderQuantizerRange, XmfVpxEncoder_Create,
    XmfVpxEncoder_CreateEx, XmfVpxEncoder_Destroy, XmfVpxEncoder_EncodeFrame, XmfVpxEncoder_Flush,
    XmfVpxEncoder_FreeEncodedFrame, XmfVpxEncoder_GetEncodedFrame, XmfVpxEncoder_GetLastCreateError,
    XmfVpxEncoder_GetLastError, XmfVpxEncoder_GetPacket, XMF_VPX_UNSET,
};

use crate::xmf::vpx::{VpxCodec, VpxError, VpxImage, VpxPacket};

#[derive(Debug, Clone, Copy)]
pub enum VpxEncoderPreset {
    Default,
    Sane,
    BestPerformance,
}

pub struct VpxEncoderBuilder {
    codec: VpxCodec,
    width: u32,
    height: u32,
    bitrate: u32,
    timebase_num: i32,
    timebase_den: i32,
    threads: u32,
    preset: VpxEncoderPreset,
    min_quantizer: Option<u8>,
    max_quantizer: Option<u8>,
}

pub struct VpxEncoder {
    // INVARIANT: A valid pointer to a properly initialized XmfVpxEncoder.
    // INVARIANT: The pointer is owned.
    ptr: *mut XmfVpxEncoder,
}

impl VpxEncoder {
    pub fn builder() -> VpxEncoderBuilder {
        VpxEncoderBuilder::new()
    }

    pub fn encode_frame(
        &mut self,
        image: &VpxImage<'_>,
        pts: i64,
        duration: usize,
        flags: u32,
    ) -> Result<(), VpxError> {
        // SAFETY: Per invariant, the pointer is valid.
        let ret = unsafe { XmfVpxEncoder_EncodeFrame(self.ptr, image.ptr, pts, duration, flags) };

        if ret != 0 {
            return Err(self.last_error());
        }

        Ok(())
    }

    pub fn next_frame(&mut self) -> Result<Option<Vec<u8>>, VpxError> {
        let mut output: *mut u8 = std::ptr::null_mut();
        let mut output_size: usize = 0;

        // SAFETY: FFI call with no outstanding precondition.
        let ret = unsafe { XmfVpxEncoder_GetEncodedFrame(self.ptr, &mut output, &mut output_size) };

        if ret == 0 {
            if output.is_null() {
                return Ok(None);
            }
            let mut vec = vec![0u8; output_size];

            // SAFETY: Safe to call since we have allocated the vector with the correct size.
            unsafe {
                std::ptr::copy_nonoverlapping(output, vec.as_mut_ptr(), output_size);
            }

            // SAFETY: safe to call as we immediately copy the data to a Vec.
            unsafe {
                XmfVpxEncoder_FreeEncodedFrame(output);
            }

            Ok(Some(vec))
        } else {
            Err(self.last_error())
        }
    }

    pub fn packet_iterator(&mut self) -> PacketIterator<'_> {
        PacketIterator::new(self)
    }

    pub fn flush(&mut self) -> Result<(), VpxError> {
        // SAFETY: FFI call with no outstanding precondition.
        let ret = unsafe { XmfVpxEncoder_Flush(self.ptr) };

        if ret == 0 {
            Ok(())
        } else {
            Err(self.last_error())
        }
    }

    fn last_error(&self) -> VpxError {
        // SAFETY: FFI call with no outstanding precondition.
        let error = unsafe { XmfVpxEncoder_GetLastError(self.ptr) };
        error.into()
    }
}

type VpxCodecIter = *const std::ffi::c_void;

pub struct PacketIterator<'encoder> {
    iter: VpxCodecIter,
    encoder: &'encoder mut VpxEncoder,
}

impl<'encoder> PacketIterator<'encoder> {
    fn new(encoder: &'encoder mut VpxEncoder) -> Self {
        Self {
            iter: std::ptr::null_mut(),
            encoder,
        }
    }
}

impl<'encoder> Iterator for PacketIterator<'encoder> {
    type Item = VpxPacket<'encoder>;

    fn next(&'_ mut self) -> Option<Self::Item> {
        // SAFETY: Per invariant, the pointer is valid.
        let ptr = unsafe { XmfVpxEncoder_GetPacket(self.encoder.ptr, &mut self.iter) };

        if ptr.is_null() {
            return None;
        }

        // SAFETY:
        // - XmfVpxEncoder_GetPacket will always return a valid pointer or null.
        // - The null case is handled above.
        let packet = unsafe { VpxPacket::from_raw(ptr) };

        if packet.is_empty() {
            return None;
        }

        Some(packet)
    }
}

impl Drop for VpxEncoder {
    fn drop(&mut self) {
        // SAFETY: VpxEncoder is owning the pointer.
        unsafe {
            XmfVpxEncoder_Destroy(self.ptr);
        };
    }
}

impl Default for VpxEncoderBuilder {
    fn default() -> Self {
        Self::new()
    }
}

impl VpxEncoderBuilder {
    pub fn new() -> Self {
        Self {
            codec: VpxCodec::VP8,
            width: 0,
            height: 0,
            bitrate: 0,
            timebase_num: 0,
            timebase_den: 0,
            threads: 0,
            preset: VpxEncoderPreset::Default,
            min_quantizer: None,
            max_quantizer: None,
        }
    }

    #[must_use]
    pub fn codec(mut self, codec: VpxCodec) -> Self {
        self.codec = codec;
        self
    }

    #[must_use]
    pub fn width(mut self, width: u32) -> Self {
        self.width = width;
        self
    }

    #[must_use]
    pub fn height(mut self, height: u32) -> Self {
        self.height = height;
        self
    }

    #[must_use]
    pub fn bitrate(mut self, bitrate: u32) -> Self {
        self.bitrate = bitrate;
        self
    }

    #[must_use]
    pub fn timebase_num(mut self, timebase_num: i32) -> Self {
        self.timebase_num = timebase_num;
        self
    }

    #[must_use]
    pub fn timebase_den(mut self, timebase_den: i32) -> Self {
        self.timebase_den = timebase_den;
        self
    }

    #[must_use]
    pub fn threads(mut self, threads: u32) -> Self {
        self.threads = threads;
        self
    }

    #[must_use]
    pub fn preset(mut self, preset: VpxEncoderPreset) -> Self {
        self.preset = preset;
        self
    }

    /// Sets the lowest (best quality) quantizer the encoder may use, in `0..=63`.
    ///
    /// Unset keeps the libvpx default. [`Self::build`] fails if it exceeds the max quantizer.
    /// Setting it requires an XMF build that exports `XmfVpxEncoder_CreateEx`.
    #[must_use]
    pub fn min_quantizer(mut self, min_quantizer: u8) -> Self {
        self.min_quantizer = Some(min_quantizer);
        self
    }

    /// Sets the highest (worst quality) quantizer the encoder may use, in `0..=63`.
    ///
    /// Unset keeps the libvpx default of 63. A lower cap keeps frames sharp when the bitrate budget is too small
    /// for them, such as sparse frames in a re-encoded recording, at the cost of a larger output.
    /// Setting it requires an XMF build that exports `XmfVpxEncoder_CreateEx`.
    #[must_use]
    pub fn max_quantizer(mut self, max_quantizer: u8) -> Self {
        self.max_quantizer = Some(max_quantizer);
        self
    }

    pub fn build(self) -> Result<VpxEncoder, VpxError> {
        let preset = match self.preset {
            VpxEncoderPreset::Default => xmf_sys::XmfVpxEncoderPreset::Default,
            VpxEncoderPreset::Sane => xmf_sys::XmfVpxEncoderPreset::Sane,
            VpxEncoderPreset::BestPerformance => xmf_sys::XmfVpxEncoderPreset::BestPerformance,
        };

        let config = XmfVpxEncoderConfig {
            codec: self.codec.into(),
            width: self.width,
            height: self.height,
            bitrate: self.bitrate,
            timebase_num: self.timebase_num,
            timebase_den: self.timebase_den,
            threads: self.threads,
            preset,
        };

        let ptr = if self.min_quantizer.is_none() && self.max_quantizer.is_none() {
            // SAFETY: FFI call with no outstanding precondition.
            unsafe { XmfVpxEncoder_Create(config) }
        } else {
            let quantizer_range = XmfVpxEncoderQuantizerRange {
                min_quantizer: self.min_quantizer.map_or(XMF_VPX_UNSET, i32::from),
                max_quantizer: self.max_quantizer.map_or(XMF_VPX_UNSET, i32::from),
                ..XmfVpxEncoderQuantizerRange::default()
            };
            // SAFETY: `quantizer_range` is a valid, initialized struct that outlives the call.
            unsafe { XmfVpxEncoder_CreateEx(config, &quantizer_range) }
        };

        if ptr.is_null() {
            // SAFETY: FFI call with no outstanding precondition.
            let error = unsafe { XmfVpxEncoder_GetLastCreateError() };
            if matches!(error.code, XmfVpxEncoderErrorCode::NoError) {
                return Err(VpxError::Internal("XmfVpxEncoder_Create(Ex) returned null"));
            }
            return Err(error.into());
        }

        Ok(VpxEncoder { ptr })
    }
}
