use core::fmt;

use xmf_sys::{
    XmfVpxCodecType, XmfVpxDecoderError, XmfVpxEncoder, XmfVpxEncoderError, XmfVpxFrame, XmfVpxFrame_Destroy,
    XmfVpxFrame_GetBuffer, XmfVpxFrame_GetDuration, XmfVpxFrame_GetFlags, XmfVpxFrame_GetHeight,
    XmfVpxFrame_GetPartitionId, XmfVpxFrame_GetPts, XmfVpxFrame_GetSize, XmfVpxFrame_GetSpatialLayerEncoded,
    XmfVpxFrame_GetWidth, XmfVpxImage, XmfVpxImage_Destroy, XmfVpxPacket, XmfVpxPacketKind, XmfVpxPacket_Destroy,
    XmfVpxPacket_GetFrame, XmfVpxPacket_GetKind, XmfVpxPacket_IsEmpty,
};

mod decoder;
mod encoder;

pub use decoder::{VpxDecoder, VpxDecoderBuilder};
pub use encoder::{PacketIterator, VpxEncoder, VpxEncoderBuilder};

#[derive(Debug, Clone, Copy)]
pub enum VpxCodec {
    VP8,
    VP9,
}

pub struct VpxImage<'decoder> {
    // INVARIANT: A valid pointer to a properly initialized XmfVpxPacket.
    // INVARIANT: The pointer is owned.
    ptr: *mut XmfVpxImage,
    // Logically holds a reference to the VpxEncoder.
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
        // SAFETY: Pointer is owned.
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

/// [`VpxEncoder`] output packet.
///
/// It contains the different kinds of output data the encoder may produce while compressing a frame.
///
/// # API Design and Safety
///
/// The [`VpxPacket`] may be invalidated as soon as the [`VpxEncoder`] which returned it is modified.
///
/// To avoid memory corruptions, we ensure the encoder is not modified by binding a logical lifetime to the encoder ('encoder).
///
/// For instance, this will not compile, as the encoder is used after the iterator is created.
///
/// ```compile_fail
///# fn example(encoder: &mut cadeau::xmf::vpx::VpxEncoder) -> Result<(), Box<dyn std::error::Error>> {
/// let mut iterator = encoder.packet_iterator();
/// let packet = iterator.next().unwrap();
/// encoder.flush();
///# Ok(())
///# }
/// ```
pub struct VpxPacket<'a> {
    // INVARIANT: A valid pointer to a properly initialized XmfVpxPacket.
    // INVARIANT: The pointer is owned.
    ptr: *mut XmfVpxPacket,
    // Logically holds a reference to the VpxEncoder.
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
        // SAFETY: Pointer is valid as the lifetime is bound to the associated encoder.
        unsafe { XmfVpxPacket_GetKind(self.ptr) }
    }

    pub fn frame(&self) -> Option<VpxFrame> {
        // SAFETY: Pointer is valid as the lifetime is bound to the associated encoder.
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
        // SAFETY: Pointer is valid as the lifetime is bound to the associated encoder.
        unsafe { XmfVpxPacket_IsEmpty(self.ptr) }
    }
}

impl Drop for VpxPacket<'_> {
    fn drop(&mut self) {
        // SAFETY:
        // - Pointer is valid as the lifetime is bound to the associated encoder.
        // - Pointer is owned.
        // - This function is not freeing the actual data held by the encoder, only the XmfVpxPacket structure.
        unsafe {
            XmfVpxPacket_Destroy(self.ptr);
        }
    }
}

pub struct VpxFrame {
    // INVARIANT: A valid pointer to a properly initialized XmfVpxFrame.
    // INVARIANT: The pointer is owned.
    ptr: *mut XmfVpxFrame,
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
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetSize(self.ptr) }
    }

    pub fn pts(&self) -> i64 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetPts(self.ptr) }
    }

    pub fn duration(&self) -> u64 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetDuration(self.ptr) }
    }

    pub fn flags(&self) -> u32 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetFlags(self.ptr) }
    }

    pub fn partition_id(&self) -> i32 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetPartitionId(self.ptr) }
    }

    pub fn width(&self, layer: i32) -> u32 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetWidth(self.ptr, layer) }
    }

    pub fn height(&self, layer: i32) -> u32 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetHeight(self.ptr, layer) }
    }

    pub fn spatial_layer_encoded(&self, layer: i32) -> u8 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfVpxFrame_GetSpatialLayerEncoded(self.ptr, layer) }
    }

    pub fn buffer(&self) -> Option<Vec<u8>> {
        let mut buffer: *const u8 = std::ptr::null();
        let mut size: usize = 0;

        // SAFETY: FFI call with no outstanding precondition.
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

impl Drop for VpxFrame {
    fn drop(&mut self) {
        // SAFETY: Pointer is owned.
        unsafe {
            XmfVpxFrame_Destroy(self.ptr);
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub enum VpxError {
    Internal(&'static str),
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
            VpxError::Internal(msg) => write!(f, "internal error (bug): {msg}"),
            VpxError::Other(msg) => write!(f, "{msg}"),
            VpxError::DecoderError(_) => write!(f, "decoder error"),
            VpxError::EncoderError(_) => write!(f, "encoder error"),
        }
    }
}

impl std::error::Error for VpxError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            VpxError::DecoderError(decoder_error) => Some(decoder_error),
            VpxError::EncoderError(encoder_error) => Some(encoder_error),
            VpxError::Other(_) => None,
            VpxError::Internal(_) => None,
        }
    }
}
