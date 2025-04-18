use core::fmt;
use std::ffi::{c_uint, c_void};
use std::fmt::Debug;

pub const VPX_EFLAG_FORCE_KF: u32 = 0x00000001;

pub type XmfVpxEncoder = c_void;
pub type XmfVpxDecoder = c_void;
pub type XmfVpxImage = c_void;
pub type XmfVpxPacket = c_void;
pub type XmfVpxFrame = c_void;
pub type VpxIterator = *const c_void;

#[repr(C)]
pub struct XmfVpXDecoder;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum XmfVpxCodecType {
    VP8,
    VP9,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct XmfVpxDecoderConfig {
    pub threads: c_uint,
    /// Width
    ///
    /// Set to 0 when unknown.
    pub w: c_uint, // Width (set to 0 if unknown)
    /// Height
    ///
    /// Set to 0 when unknown.
    pub h: c_uint,
    pub codec: XmfVpxCodecType,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum XmfVpxDecoderErrorCode {
    NoError = 0,
    MemoryError = 1,
    InitError = 2,
    DecodeError = 3,
    NoFrameAvailable = 4,
    VpxError = 5,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum VpxCodecError {
    /// Operation completed without error
    VpxCodecOk,
    /// Unspecified error
    VpxCodecError,
    /// Memory operation failed
    VpxCodecMemError,
    /// ABI version mismatch
    VpxCodecAbiMismatch,
    /// Algorithm does not have required capability
    VpxCodecIncapable,
    /// The given bitstream is not supported
    VpxCodecUnsupBitstream,
    /// Encoded bitstream uses an unsupported feature
    VpxCodecUnsupFeature,
    /// The coded data for this stream is corrupt or incomplete
    VpxCodecCorruptFrame,
    /// An application-supplied parameter is not valid
    VpxCodecInvalidParam,
    /// An iterator reached the end of list
    VpxCodecListEnd,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct VpxErrorDetail {
    pub error_code: VpxCodecError,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union XmfVpxDecoderErrorDetail {
    pub vpx_error: VpxErrorDetail,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XmfVpxDecoderError {
    pub code: XmfVpxDecoderErrorCode,
    pub detail: XmfVpxDecoderErrorDetail,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct XmfVpxEncoderConfig {
    pub codec: XmfVpxCodecType,
    pub width: u32,
    pub height: u32,
    pub bitrate: u32,
    pub timebase_num: i32,
    pub timebase_den: i32,
    pub threads: u32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum XmfVpxEncoderErrorCode {
    NoError,
    MemoryError,
    VpxError,
    InvalidParam,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union XmfVpxEncoderErrorDetail {
    pub vpx_error: VpxErrorDetail,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XmfVpxEncoderError {
    pub code: XmfVpxEncoderErrorCode,
    pub detail: XmfVpxEncoderErrorDetail,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum XmfVpxPacketKind {
    CodecCxFramePkt,
    CodecStatsPkt,
    CodecFpmbStatsPkt,
    CodecPsnrPkt,
    CodecCustomPkt = 256,
}

impl std::error::Error for XmfVpxDecoderError {}

impl fmt::Display for XmfVpxDecoderError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if matches!(self.code, XmfVpxDecoderErrorCode::VpxError) {
            // SAFETY: The union detail.vpx_error is always valid when the error code is VpxError.
            unsafe { write!(f, "VPX error: {:?}", self.detail.vpx_error.error_code) }
        } else {
            write!(f, "XMF VPX decoder error: {:?}", self.code)
        }
    }
}

impl Debug for XmfVpxDecoderError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if matches!(self.code, XmfVpxDecoderErrorCode::VpxError) {
            // SAFETY: The union detail.vpx_error is always valid when the error code is VpxError.
            unsafe {
                f.debug_struct("XmfVpxDecoderError")
                    .field("code", &self.code)
                    .field("detail", &self.detail.vpx_error.error_code)
                    .finish()
            }
        } else {
            f.debug_struct("XmfVpxDecoderError").field("code", &self.code).finish()
        }
    }
}

impl std::error::Error for XmfVpxEncoderError {}

impl fmt::Display for XmfVpxEncoderError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if matches!(self.code, XmfVpxEncoderErrorCode::VpxError) {
            // SAFETY: The union detail.vpx_error is always valid when the error code is VpxErr:19or.
            unsafe { write!(f, "VPX error: {:?}", self.detail.vpx_error.error_code) }
        } else {
            write!(f, "XMF VPX encoder error: {:?}", self.code)
        }
    }
}

impl Debug for XmfVpxEncoderError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if matches!(self.code, XmfVpxEncoderErrorCode::VpxError) {
            // SAFETY: The union detail.vpx_error is always valid when the error code is VpxError.
            unsafe {
                f.debug_struct("XmfVpxEncoderError")
                    .field("code", &self.code)
                    .field("detail", &self.detail.vpx_error.error_code)
                    .finish()
            }
        } else {
            f.debug_struct("XmfVpxEncoderError").field("code", &self.code).finish()
        }
    }
}
