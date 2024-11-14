use core::fmt;

use xmf_sys::{
    XmfVpxCodecType, XmfVpxDecoderError, XmfVpxEncoderError, XmfVpxFrame, XmfVpxFrame_Destroy, XmfVpxFrame_GetBuffer,
    XmfVpxFrame_GetDuration, XmfVpxFrame_GetFlags, XmfVpxFrame_GetHeight, XmfVpxFrame_GetPartitionId,
    XmfVpxFrame_GetPts, XmfVpxFrame_GetSize, XmfVpxFrame_GetSpatialLayerEncoded, XmfVpxFrame_GetWidth, XmfVpxImage,
    XmfVpxImage_Destroy, XmfVpxPacket, XmfVpxPacketKind, XmfVpxPacket_Destroy, XmfVpxPacket_GetFrame,
    XmfVpxPacket_GetKind, XmfVpxPacket_IsEmpty,
};

pub mod decoder;
pub mod encoder;

#[derive(Debug, Clone, Copy)]
pub enum VpxCodec {
    VP8,
    VP9,
}

pub struct VpxImage {
    pub(crate) ptr: *mut XmfVpxImage,
}

impl Drop for VpxImage {
    fn drop(&mut self) {
        // Safety: it is safe to call, the owenership of the pointer is managed by the XmfVpxImage itself.
        unsafe {
            XmfVpxImage_Destroy(self.ptr);
        }
    }
}

impl From<VpxCodec> for XmfVpxCodecType {
    fn from(codec: VpxCodec) -> Self {
        match codec {
            VpxCodec::VP8 => XmfVpxCodecType::VP8,
            VpxCodec::VP9 => XmfVpxCodecType::VP9,
        }
    }
}

pub fn is_key_frame(buffer: &[u8]) -> bool {
    if buffer.is_empty() {
        return false;
    }
    buffer[0] & 0x1 == 0
}

pub struct VpxPacket {
    pub(crate) ptr: *mut XmfVpxPacket,
}

impl VpxPacket {
    /// # Safety
    ///
    /// This function is `unsafe` because it assumes that `self.ptr` is a valid pointer.
    /// It should not be used after any call to a `VpxEncoder` method that modifies or releases the packet.
    pub unsafe fn kind(&self) -> XmfVpxPacketKind {
        // Safety: It's up to the caller to ensure the packet is valid.
        unsafe { XmfVpxPacket_GetKind(self.ptr) }
    }

    /// # Safety
    ///
    /// Obtain the deep copy of the frame from the packet, must be used before any call to VpxEncoder method.
    /// The frame produced by this method is always valid.
    /// This function is `unsafe` because it assumes that `self.ptr` is a valid pointer.
    /// It should not be used after any call to a `VpxEncoder` method that modifies or releases the packet.
    pub unsafe fn frame(&self) -> Option<VpxFrame> {
        // Safety: it's up to the caller to ensure the packet is valid, the frame will remain valid until the frame itself is destroyed.
        let frame_ptr = unsafe { XmfVpxPacket_GetFrame(self.ptr) };
        if frame_ptr.is_null() {
            None
        } else {
            Some(VpxFrame { ptr: frame_ptr })
        }
    }

    /// # Safety
    ///
    /// This function is `unsafe` because it assumes that `self.ptr` is a valid pointer.
    /// It should not be used after any call to a `VpxEncoder` method that modifies or releases the packet.
    pub unsafe fn is_empty(&self) -> bool {
        // Safety: It's up to the caller to ensure the packet is valid.
        unsafe { XmfVpxPacket_IsEmpty(self.ptr) }
    }
}

impl Drop for VpxPacket {
    fn drop(&mut self) {
        // Safety: XmfVpxPacket pointer is owned by the VpxPacket, the inner pointer of XmfVpxPacket is managed by vpx_encoder, however
        // The `XmfVpxPacket_Destroy` will not touch the inner pointer managed by the encoder, so this functino is safe to call.
        unsafe {
            XmfVpxPacket_Destroy(self.ptr);
        }
    }
}

pub struct VpxFrame {
    ptr: *mut XmfVpxFrame,
}

impl Drop for VpxFrame {
    fn drop(&mut self) {
        // Safety: see Safety Note in VpxFrame::new, this is safe to call.
        unsafe {
            XmfVpxFrame_Destroy(self.ptr);
        }
    }
}

impl VpxFrame {
    pub fn size(&self) -> usize {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetSize(self.ptr) }
    }

    pub fn pts(&self) -> i64 {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetPts(self.ptr) }
    }

    pub fn duration(&self) -> u64 {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetDuration(self.ptr) }
    }

    pub fn flags(&self) -> u32 {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetFlags(self.ptr) }
    }

    pub fn partition_id(&self) -> i32 {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetPartitionId(self.ptr) }
    }

    pub fn width(&self, layer: i32) -> u32 {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetWidth(self.ptr, layer) }
    }

    pub fn height(&self, layer: i32) -> u32 {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetHeight(self.ptr, layer) }
    }

    pub fn spatial_layer_encoded(&self, layer: i32) -> u8 {
        // Safety: The pointer is valid and the function is safe to call. See Safety Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetSpatialLayerEncoded(self.ptr, layer) }
    }

    pub fn buffer(&self) -> Option<Vec<u8>> {
        let mut buffer: *const u8 = std::ptr::null();
        let mut size: usize = 0;
        // Safety: We are pretty sure the frame is valid, and so is the buffer, since frame is a deep copy of the packet.
        let result = unsafe { XmfVpxFrame_GetBuffer(self.ptr, &mut buffer, &mut size) };
        if result == 0 && !buffer.is_null() {
            let mut vec = vec![0u8; size];
            // Safety: Copying the buffer to the vec is safe, since the buffer is valid and the size is correct.
            unsafe {
                std::ptr::copy_nonoverlapping(buffer, vec.as_mut_ptr(), size);
            }
            Some(vec)
        } else {
            None
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub enum VpxError {
    NullPointer,
    Other(&'static str),
    DecoderError(XmfVpxDecoderError),
    EncoderError(XmfVpxEncoderError),
}

impl From<XmfVpxDecoderError> for VpxError {
    fn from(error: XmfVpxDecoderError) -> Self {
        VpxError::DecoderError(error)
    }
}

impl From<XmfVpxEncoderError> for VpxError {
    fn from(error: XmfVpxEncoderError) -> Self {
        VpxError::EncoderError(error)
    }
}

impl From<&'static str> for VpxError {
    fn from(error: &'static str) -> Self {
        VpxError::Other(error)
    }
}

impl fmt::Display for VpxError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            VpxError::NullPointer => write!(f, "Null pointer error"),
            VpxError::Other(msg) => write!(f, "Other error: {}", msg),
            VpxError::DecoderError(err) => write!(f, "Decoder error: {:?}", err),
            VpxError::EncoderError(err) => write!(f, "Encoder error: {:?}", err),
        }
    }
}

impl std::error::Error for VpxError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            VpxError::DecoderError(decoder_error) => Some(decoder_error),
            VpxError::EncoderError(encoder_error) => Some(encoder_error),
            VpxError::Other(_) => None,
            VpxError::NullPointer => None,
        }
    }
}
