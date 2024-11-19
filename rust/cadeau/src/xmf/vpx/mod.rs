use core::fmt;

use decoder::VpxDecoder;
use xmf_sys::{
    XmfVpxCodecType, XmfVpxDecoderError, XmfVpxEncoder, XmfVpxEncoderError, XmfVpxFrame, XmfVpxFrame_Destroy, XmfVpxFrame_GetBuffer, XmfVpxFrame_GetDuration, XmfVpxFrame_GetFlags, XmfVpxFrame_GetHeight, XmfVpxFrame_GetPartitionId, XmfVpxFrame_GetPts, XmfVpxFrame_GetSize, XmfVpxFrame_GetSpatialLayerEncoded, XmfVpxFrame_GetWidth, XmfVpxImage, XmfVpxImage_Destroy, XmfVpxPacket, XmfVpxPacketKind, XmfVpxPacket_Destroy, XmfVpxPacket_GetFrame, XmfVpxPacket_GetKind, XmfVpxPacket_IsEmpty
};

pub mod decoder;
pub mod encoder;

#[derive(Debug, Clone, Copy)]
pub enum VpxCodec {
    VP8,
    VP9,
}
pub struct VpxImage<'decoder> {
    ptr: *mut XmfVpxImage,
    _marker: std::marker::PhantomData<&'decoder mut VpxDecoder>,
}

impl VpxImage<'_> {
    /// # SAFETY
    ///
    /// The pointer must be valid and must not be null.
    unsafe fn from_raw(ptr: *mut XmfVpxImage) -> Self {
        VpxImage {
            ptr,
            _marker: std::marker::PhantomData,
        }
    }
}

impl Drop for VpxImage<'_> {
    fn drop(&mut self) {
        // SAFETY: it is safe to call, the pointer is owned by the VpxImage itself.
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

/// SAFETY Note: The packet is only valid before the next call to any function on the encoder.
/// The Packet iterator holds a mutable reference to the encoder,
/// While the iterator is alive, the encoder with mutable reference is not allowed to be used.
/// Hence all the functions on the packet are safe to call.
///
/// ex.
///
/// ```rust
/// // This will not compile, as packet outlives the iterator.
/// fn example(&mut self) {
///     let mut packet;
///     {
///         let mut iterator = self.encoder.packet_iterator();
///         packet = iterator.next().unwrap();
///     }
/// }
/// ```
/// // This will not compile as well, as the encoder is used after the iterator is created.
/// ```rust
/// fn example(&mut self) {
///     let mut iterator = self.encoder.packet_iterator();
///     let packet = iterator.next().unwrap();
///     self.encoder.flush();
/// }
/// ```
pub struct VpxPacket<'a> {
    ptr: *mut XmfVpxPacket,
    _marker: std::marker::PhantomData<&'a mut XmfVpxEncoder>
}

impl VpxPacket<'_> {
    /// # SAFETY
    ///
    /// The pointer must be valid and must not be null.
    pub(crate) unsafe fn from_raw(ptr: *mut XmfVpxPacket) -> Self {
        VpxPacket {
            ptr,
            _marker: std::marker::PhantomData,
        }
    }

    pub fn kind(&self) -> XmfVpxPacketKind {
        // SAFETY: see SAFETY Note
        unsafe { XmfVpxPacket_GetKind(self.ptr) }
    }

    pub fn frame(&self) -> Option<VpxFrame> {
        // SAFETY: see SAFETY Note in VpxPacket
        let frame_ptr = unsafe { XmfVpxPacket_GetFrame(self.ptr) };
        if frame_ptr.is_null() {
            None
        } else {
            // SAFETY: The frame pointer is not null and is valid, since the packet is valid.
            Some(unsafe { VpxFrame::from_raw(frame_ptr) })
        }
    }

    pub fn is_empty(&self) -> bool {
        // SAFETY: see SAFETY Note in VpxPacket
        unsafe { XmfVpxPacket_IsEmpty(self.ptr) }
    }
}

impl Drop for VpxPacket<'_> {
    fn drop(&mut self) {
        // SAFETY: See the SAFETY Note in VpxPacket
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
        // SAFETY: see SAFETY Note in VpxFrame::new, this is safe to call.
        unsafe {
            XmfVpxFrame_Destroy(self.ptr);
        }
    }
}

impl VpxFrame {
    /// # SAFETY
    /// The pointer must be valid and must not be null.
    unsafe fn from_raw(ptr: *mut XmfVpxFrame) -> Self {
        VpxFrame { ptr }
    }

    pub fn size(&self) -> usize {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetSize(self.ptr) }
    }

    pub fn pts(&self) -> i64 {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetPts(self.ptr) }
    }

    pub fn duration(&self) -> u64 {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetDuration(self.ptr) }
    }

    pub fn flags(&self) -> u32 {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetFlags(self.ptr) }
    }

    pub fn partition_id(&self) -> i32 {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetPartitionId(self.ptr) }
    }

    pub fn width(&self, layer: i32) -> u32 {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetWidth(self.ptr, layer) }
    }

    pub fn height(&self, layer: i32) -> u32 {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetHeight(self.ptr, layer) }
    }

    pub fn spatial_layer_encoded(&self, layer: i32) -> u8 {
        // SAFETY: The pointer is valid and the function is safe to call. See SAFETY Note in VpxFrame::new.
        unsafe { XmfVpxFrame_GetSpatialLayerEncoded(self.ptr, layer) }
    }

    pub fn buffer(&self) -> Option<Vec<u8>> {
        let mut buffer: *const u8 = std::ptr::null();
        let mut size: usize = 0;
        // SAFETY: We are pretty sure the frame is valid, and so is the buffer, since frame is a deep copy of the packet.
        let result = unsafe { XmfVpxFrame_GetBuffer(self.ptr, &mut buffer, &mut size) };
        if result == 0 && !buffer.is_null() {
            let mut vec = vec![0u8; size];
            // SAFETY: Copying the buffer to the vec is safe, since the buffer is valid and the size is correct.
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
