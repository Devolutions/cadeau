use core::fmt;
use core::mem::MaybeUninit;
use std::ffi::c_int;

use xmf_sys::{
    XmfVpxCodecType, XmfVpxDecoderError, XmfVpxEncoder, XmfVpxEncoderError, XmfVpxFrame, XmfVpxFrame_Destroy,
    XmfVpxFrame_GetBuffer, XmfVpxFrame_GetDuration, XmfVpxFrame_GetFlags, XmfVpxFrame_GetHeight,
    XmfVpxFrame_GetPartitionId, XmfVpxFrame_GetPts, XmfVpxFrame_GetSize, XmfVpxFrame_GetSpatialLayerEncoded,
    XmfVpxFrame_GetWidth, XmfVpxImage, XmfVpxImage_Destroy, XmfVpxImage_GetColorRange, XmfVpxImage_GetColorSpace,
    XmfVpxImage_GetFormat, XmfVpxImage_GetHeight, XmfVpxImage_GetPlane, XmfVpxImage_GetStride, XmfVpxImage_GetWidth,
    XmfVpxPacket, XmfVpxPacketKind, XmfVpxPacket_Destroy, XmfVpxPacket_GetFrame, XmfVpxPacket_GetKind,
    XmfVpxPacket_IsEmpty, VPX_CR_FULL_RANGE, VPX_CR_STUDIO_RANGE, VPX_CS_BT_2020, VPX_CS_BT_601, VPX_CS_BT_709,
    VPX_CS_RESERVED, VPX_CS_SMPTE_170, VPX_CS_SMPTE_240, VPX_CS_SRGB, VPX_CS_UNKNOWN, VPX_IMG_FMT_I420, VPX_PLANE_U,
    VPX_PLANE_V, VPX_PLANE_Y,
};

mod decoder;
mod encoder;

#[cfg(test)]
mod tests;

pub use decoder::{VpxDecoder, VpxDecoderBuilder};
pub use encoder::{PacketIterator, VpxEncoder, VpxEncoderBuilder, VpxEncoderPreset};

#[derive(Debug, Clone, Copy)]
pub enum VpxCodec {
    VP8,
    VP9,
}

/// libvpx `vpx_img_fmt_t` of a decoded image.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct VpxImageFormat(pub i32);

impl VpxImageFormat {
    /// 8-bit planar YUV 4:2:0.
    pub const I420: Self = Self(VPX_IMG_FMT_I420);
}

/// libvpx `vpx_color_space_t` reported by the decoder.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct VpxColorSpace(pub i32);

impl VpxColorSpace {
    pub const UNKNOWN: Self = Self(VPX_CS_UNKNOWN);
    pub const BT_601: Self = Self(VPX_CS_BT_601);
    pub const BT_709: Self = Self(VPX_CS_BT_709);
    pub const SMPTE_170: Self = Self(VPX_CS_SMPTE_170);
    pub const SMPTE_240: Self = Self(VPX_CS_SMPTE_240);
    pub const BT_2020: Self = Self(VPX_CS_BT_2020);
    pub const RESERVED: Self = Self(VPX_CS_RESERVED);
    pub const SRGB: Self = Self(VPX_CS_SRGB);
}

/// libvpx `vpx_color_range_t` reported by the decoder.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct VpxColorRange(pub i32);

impl VpxColorRange {
    /// Studio (limited) range: Y in 16..=235, U and V in 16..=240.
    pub const STUDIO: Self = Self(VPX_CR_STUDIO_RANGE);
    /// Full range: Y, U and V in 0..=255.
    pub const FULL: Self = Self(VPX_CR_FULL_RANGE);
}

/// One plane of a decoded image, borrowed until the next decoder call.
#[derive(Clone, Copy)]
pub struct VpxPlane<'image> {
    data: &'image [MaybeUninit<u8>],
    columns: usize,
    stride: usize,
}

impl<'image> VpxPlane<'image> {
    /// Borrows each row's visible bytes without exposing padding that libvpx may leave uninitialized.
    pub fn rows(&self) -> impl ExactSizeIterator<Item = &'image [u8]> {
        let columns = self.columns;
        self.data.chunks(self.stride).map(move |row| {
            // SAFETY: Each chunk starts at a row whose first `columns` bytes are initialized pixels.
            unsafe { core::slice::from_raw_parts(row.as_ptr().cast(), columns) }
        })
    }

    /// Number of pixel bytes in each row.
    pub fn width(&self) -> usize {
        self.columns
    }

    /// Number of rows.
    pub fn height(&self) -> usize {
        self.data.len().div_ceil(self.stride)
    }

    /// Distance in bytes between the starts of two rows. At least [`Self::width`].
    pub fn stride(&self) -> usize {
        self.stride
    }

    /// Returns a pointer to the first pixel of the plane, for code that takes a base pointer and a stride, such as a
    /// C or SIMD color converter.
    ///
    /// Row `r` starts at `as_ptr().add(r * stride())`, and its first [`width()`](Self::width) bytes are pixels.
    /// Use [`Self::rows`] instead when a slice per row is enough: it needs no `unsafe`.
    ///
    /// # Safety
    ///
    /// The returned pointer carries neither the image's lifetime nor any promise that every byte is initialized, so
    /// the caller must uphold all of the following:
    ///
    /// - Only read through it. The buffer belongs to the decoder.
    /// - Only read the first [`width()`](Self::width) bytes of each of the [`height()`](Self::height) rows. The
    ///   bytes between rows are padding that libvpx may leave uninitialized; reading them is undefined behavior.
    /// - Stop using it before the image is dropped. Until then the decoder stays mutably borrowed, so it cannot
    ///   decode again or be dropped; keep the image (or this plane) alive for as long as the pointer is in use.
    ///
    /// # Example
    ///
    /// ```rust,no_run
    /// use cadeau::xmf::vpx::VpxPlane;
    ///
    /// fn luma_sum(plane: &VpxPlane<'_>) -> u64 {
    ///     // SAFETY: Only pixel bytes are read below, while `plane` keeps the image borrowed.
    ///     let base = unsafe { plane.as_ptr() };
    ///     let mut sum = 0;
    ///     for row in 0..plane.height() {
    ///         // SAFETY: Row `row` starts `row * stride` bytes after `base`, inside the plane.
    ///         let start = unsafe { base.add(row * plane.stride()) };
    ///         // SAFETY: The row has `width` initialized pixels.
    ///         let pixels = unsafe { core::slice::from_raw_parts(start, plane.width()) };
    ///         sum += pixels.iter().map(|&pixel| u64::from(pixel)).sum::<u64>();
    ///     }
    ///     sum
    /// }
    /// ```
    pub unsafe fn as_ptr(&self) -> *const u8 {
        self.data.as_ptr().cast()
    }
}

impl fmt::Debug for VpxPlane<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // The pixels are not printed: a 1080p luma plane alone is two million bytes, some of them uninitialized.
        f.debug_struct("VpxPlane")
            .field("width", &self.width())
            .field("height", &self.height())
            .field("stride", &self.stride)
            .finish_non_exhaustive()
    }
}

/// The three planes of an 8-bit I420 image, borrowed until the next decoder call.
#[derive(Debug, Clone, Copy)]
pub struct VpxI420Planes<'image> {
    pub y: VpxPlane<'image>,
    pub u: VpxPlane<'image>,
    pub v: VpxPlane<'image>,
}

pub struct VpxImage<'decoder> {
    // INVARIANT: A valid pointer to a properly initialized XmfVpxImage.
    // INVARIANT: The pointer is owned.
    ptr: *mut XmfVpxImage,
    // Logically holds a reference to the VpxDecoder.
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

    pub fn width(&self) -> u32 {
        // SAFETY: Pointer is valid as the lifetime is bound to the associated decoder.
        unsafe { XmfVpxImage_GetWidth(self.ptr) }
    }

    pub fn height(&self) -> u32 {
        // SAFETY: Pointer is valid as the lifetime is bound to the associated decoder.
        unsafe { XmfVpxImage_GetHeight(self.ptr) }
    }

    /// libvpx `vpx_img_fmt_t` of the decoded image.
    pub fn format(&self) -> VpxImageFormat {
        // SAFETY: Pointer is valid as the lifetime is bound to the associated decoder.
        VpxImageFormat(unsafe { XmfVpxImage_GetFormat(self.ptr) })
    }

    /// Color space reported by the decoder. VP8 always reports [`VpxColorSpace::UNKNOWN`].
    pub fn color_space(&self) -> VpxColorSpace {
        // SAFETY: Pointer is valid as the lifetime is bound to the associated decoder.
        VpxColorSpace(unsafe { XmfVpxImage_GetColorSpace(self.ptr) })
    }

    /// Color range reported by the decoder. VP8 always reports [`VpxColorRange::STUDIO`].
    pub fn color_range(&self) -> VpxColorRange {
        // SAFETY: Pointer is valid as the lifetime is bound to the associated decoder.
        VpxColorRange(unsafe { XmfVpxImage_GetColorRange(self.ptr) })
    }

    /// Borrows the Y, U and V planes, or returns `None` when the image is not 8-bit I420.
    pub fn i420_planes(&self) -> Option<VpxI420Planes<'_>> {
        if self.format() != VpxImageFormat::I420 {
            return None;
        }

        let width = usize::try_from(self.width()).ok()?;
        let height = usize::try_from(self.height()).ok()?;
        if width == 0 || height == 0 {
            return None;
        }

        let chroma_width = width.div_ceil(2);
        let chroma_height = height.div_ceil(2);

        // SAFETY: The image is 8-bit I420, so libvpx allocated `height` luma rows of at least `width` bytes.
        let y = unsafe { self.plane(VPX_PLANE_Y, width, height) }?;
        // SAFETY: The image is 8-bit I420, so libvpx allocated `(height + 1) / 2` rows of at least
        // `(width + 1) / 2` bytes for each chroma plane.
        let u = unsafe { self.plane(VPX_PLANE_U, chroma_width, chroma_height) }?;
        // SAFETY: Same as for the U plane.
        let v = unsafe { self.plane(VPX_PLANE_V, chroma_width, chroma_height) }?;

        Some(VpxI420Planes { y, u, v })
    }

    /// Borrows `rows` rows of `columns` bytes from a plane, or returns `None` when the plane is unavailable.
    ///
    /// # Safety
    ///
    /// Plane `index` must be one allocation holding `rows` rows at the reported stride, with at least `columns`
    /// initialized pixel bytes per row. The decoder must not change the buffer while the image is borrowed.
    unsafe fn plane(&self, index: c_int, columns: usize, rows: usize) -> Option<VpxPlane<'_>> {
        // SAFETY: Pointer is valid as the lifetime is bound to the associated decoder.
        let data = unsafe { XmfVpxImage_GetPlane(self.ptr, index) };
        // SAFETY: Pointer is valid as the lifetime is bound to the associated decoder.
        let stride = unsafe { XmfVpxImage_GetStride(self.ptr, index) };
        let stride = usize::try_from(stride).ok().filter(|&stride| stride >= columns)?;
        if data.is_null() {
            return None;
        }

        let length = rows.checked_sub(1)?.checked_mul(stride)?.checked_add(columns)?;
        isize::try_from(length).ok()?;

        // SAFETY: The caller guarantees this allocation and borrow. MaybeUninit allows uninitialized row padding;
        // only the initialized pixel bytes are exposed by `rows()`.
        let data = unsafe { core::slice::from_raw_parts(data.cast::<MaybeUninit<u8>>(), length) };

        Some(VpxPlane { data, columns, stride })
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
/// To avoid memory corruptions, we ensure the encoder is not modified by binding a logical lifetime to the encoder (`'encoder`).
///
/// For instance, this will not compile, as the encoder is used after the iterator is created.
///
/// ```compile_fail
/// fn example(encoder: &mut cadeau::xmf::vpx::VpxEncoder) {
///     let mut iterator = encoder.packet_iterator();
///     let packet = iterator.next().unwrap();
///     encoder.flush();
/// }
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
