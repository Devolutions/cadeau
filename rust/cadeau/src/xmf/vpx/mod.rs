use core::fmt;

use decoder::VpxDecoder;
use xmf_sys::{
    XmfVpxCodecType, XmfVpxDecoderError, XmfVpxEncoder, XmfVpxEncoderError, XmfVpxFrame, XmfVpxFrame_Destroy,
    XmfVpxFrame_GetBuffer, XmfVpxFrame_GetDuration, XmfVpxFrame_GetFlags, XmfVpxFrame_GetHeight,
    XmfVpxFrame_GetPartitionId, XmfVpxFrame_GetPts, XmfVpxFrame_GetSize, XmfVpxFrame_GetSpatialLayerEncoded,
    XmfVpxFrame_GetWidth, XmfVpxImage, XmfVpxImage_Destroy, XmfVpxPacket, XmfVpxPacketKind, XmfVpxPacket_Destroy,
    XmfVpxPacket_GetFrame, XmfVpxPacket_GetKind, XmfVpxPacket_IsEmpty,
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
    // Hold a reference to the decoder, so method requires &mut decoder will not be allowed.
    _marker: std::marker::PhantomData<&'decoder VpxDecoder>,
}

impl VpxImage<'_> {
    /// # Safety
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
        // SAFETY: XmfVpxImage_Destroy requires no preconditions, it is safe to call. Even if the pointer is null.
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
    // Hold a reference to the encoder, so method requires &mut encoder will not be allowed.
    _marker: std::marker::PhantomData<&'a XmfVpxEncoder>,
}

impl VpxPacket<'_> {
    /// # Safety
    ///
    /// - The pointer must be valid.
    /// - The pointer must be obtained from the PacketIterator.
    pub(crate) unsafe fn from_raw(ptr: *mut XmfVpxPacket) -> Self {
        VpxPacket {
            ptr,
            _marker: std::marker::PhantomData,
        }
    }

    pub fn kind(&self) -> XmfVpxPacketKind {
        // SAFETY: We are sure the pointer is valid, since it's lifetime is tied to the packet iterator.
        // and which is further tied to the encoder. See documentation on `VpxPacket` for more details.
        unsafe { XmfVpxPacket_GetKind(self.ptr) }
    }

    pub fn frame(&self) -> Option<VpxFrame> {
        // SAFETY: We are sure the pointer is valid, since it's lifetime is tied to the packet iterator.
        // and which is further tied to the encoder. See documentation on `VpxPacket` for more details.
        let frame_ptr = unsafe { XmfVpxPacket_GetFrame(self.ptr) };
        if frame_ptr.is_null() {
            None
        } else {
            // SAFETY: We verified the pointer is not null.
            // When non null, the pointer returned by XmfVpxPacket_GetFrame is always valid and owned.
            // XmfVpxPacket_GetFrame is performing a deep copy of the XmfVpxPacket data into the frame data, which must be freed separately via XmfVpxFrame_Destroy
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
        // SAFETY: XmfVpxPacket_Destroy safe to call even if the pointer is null.
        // 
        // note: The pointer in side `XmfVpxPacket` is owned by the encoder and will not be freed by this funciton
        // this function will only free the pointer reference to the packet.
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
        // SAFETY: self.ptr is always owned by the VpxFrame, and hence it is safe to call XmfVpxFrame_Destroy.
        unsafe {
            XmfVpxFrame_Destroy(self.ptr);
        }
    }
}

impl VpxFrame {
    /// # Safety
    ///
    /// - The pointer must be valid.
    /// - The pointer must be owned. (`VpxFrame` is responsible for freeing the resource.)
    /// - The pointed `XmfVpxFrame` must be owning the internal data.
    unsafe fn from_raw(ptr: *mut XmfVpxFrame) -> Self {
        VpxFrame { ptr }
    }

    pub fn size(&self) -> usize {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetSize(self.ptr) }
    }

    pub fn pts(&self) -> i64 {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetPts(self.ptr) }
    }

    pub fn duration(&self) -> u64 {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetDuration(self.ptr) }
    }

    pub fn flags(&self) -> u32 {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetFlags(self.ptr) }
    }

    pub fn partition_id(&self) -> i32 {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetPartitionId(self.ptr) }
    }

    pub fn width(&self, layer: i32) -> u32 {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetWidth(self.ptr, layer) }
    }

    pub fn height(&self, layer: i32) -> u32 {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetHeight(self.ptr, layer) }
    }

    pub fn spatial_layer_encoded(&self, layer: i32) -> u8 {
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
        unsafe { XmfVpxFrame_GetSpatialLayerEncoded(self.ptr, layer) }
    }

    pub fn buffer(&self) -> Option<Vec<u8>> {
        let mut buffer: *const u8 = std::ptr::null();
        let mut size: usize = 0;
        // SAFETY: This pointer is always valid, since it is owned by the VpxFrame.
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
