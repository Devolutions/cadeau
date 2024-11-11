
pub const VPX_EFLAG_FORCE_KF: u32 = 0x00000001;

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
    pub threads: u32, // Corresponds to 'unsigned int' in C
    pub w: u32,       // Width (set to 0 if unknown)
    pub h: u32,       // Height (set to 0 if unknown)
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
pub struct VpxErrorDetail {
    pub error_code: i32, // vpx_codec_err_t assumed to be an integer
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
pub struct XmfVpxEncoder;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct XmfVpxEncoderConfig {
    pub codec: XmfVpxCodecType,
    pub width: u32,
    pub height: u32,
    pub bitrate: u32,
    pub timebase_num: u32,
    pub timebase_den: u32,
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
