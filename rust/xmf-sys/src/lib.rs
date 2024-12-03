#![doc = include_str!("../README.md")]
#![allow(clippy::too_many_arguments)]
#![allow(non_snake_case)]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]

mod macros;
mod vpx;

use std::ffi::{c_char, c_int, c_void};

pub type XmfBipBuffer = c_void;

pub type XmfWebM = c_void;

pub type XmfRecorder = c_void;

pub type XmfWebMMuxer = c_void;

pub const XMF_MUXER_FILE_OPEN_ERROR: c_int = -1001;
pub const XMF_MUXER_PARSER_ERROR: c_int = -1002;
pub const XMF_MUXER_MUXER_ERROR: c_int = -1003;

#[doc(inline)]
pub use raw::global::*;
pub use vpx::*;

pub mod raw {
    use super::*;

    crate::macros::external_library!(feature = "dlopen", Api, "xmf",
        functions:
            // BipBuffer
            fn XmfBipBuffer_Grow(ctx: *mut XmfBipBuffer, size: usize) -> bool,
            fn XmfBipBuffer_Clear(ctx: *mut XmfBipBuffer) -> (),
            fn XmfBipBuffer_UsedSize(ctx: *mut XmfBipBuffer) -> usize,
            fn XmfBipBuffer_BufferSize(ctx: *mut XmfBipBuffer) -> usize,
            fn XmfBipBuffer_Read(ctx: *mut XmfBipBuffer, data: *mut u8, size: usize) -> c_int,
            fn XmfBipBuffer_Write(ctx: *mut XmfBipBuffer, data: *const u8, size: usize) -> c_int,
            fn XmfBipBuffer_New(size: usize) -> *mut XmfBipBuffer,
            fn XmfBipBuffer_Free(ctx: *mut XmfBipBuffer) -> (),

            // Recorder
            fn XmfRecorder_UpdateFrame(ctx: *mut XmfRecorder, buffer: *const u8, updateX: u32, updateY: u32, updateWidth: u32, updateHeight: u32, surfaceStep: u32) -> (),
            fn XmfRecorder_GetTimeout(ctx: *mut XmfRecorder) -> u32,
            fn XmfRecorder_Timeout(ctx: *mut XmfRecorder) -> (),
            fn XmfRecorder_SetMinimumFrameRate(ctx: *mut XmfRecorder, frameRate: u32) -> (),
            fn XmfRecorder_GetFrameRate(ctx: *mut XmfRecorder) -> u32,
            fn XmfRecorder_SetFrameRate(ctx: *mut XmfRecorder, frameRate: u32) -> (),
            fn XmfRecorder_SetFrameSize(ctx: *mut XmfRecorder, frameWidth: u32, frameHeight: u32) -> (),
            fn XmfRecorder_SetVideoQuality(ctx: *mut XmfRecorder, videoQuality: u32) -> (),
            fn XmfRecorder_SetCurrentTime(ctx: *mut XmfRecorder, currentTime: u64) -> (),
            fn XmfRecorder_GetCurrentTime(ctx: *mut XmfRecorder) -> u64,
            fn XmfRecorder_SetFileName(ctx: *mut XmfRecorder, filename: *const c_char) -> (),
            fn XmfRecorder_SetBipBuffer(ctx: *mut XmfRecorder, bb: *mut XmfBipBuffer) -> (),
            fn XmfRecorder_Init(ctx: *mut XmfRecorder) -> bool,
            fn XmfRecorder_Uninit(ctx: *mut XmfRecorder) -> (),
            fn XmfRecorder_New() -> *mut XmfRecorder,
            fn XmfRecorder_Free(ctx: *mut XmfRecorder) -> (),

            // Muxer
            fn XmfWebMMuxer_Remux(ctx: *mut XmfWebMMuxer, inputPath: *const c_char, outputPath: *const c_char) -> c_int,
            fn XmfWebMMuxer_New() -> *mut XmfWebMMuxer,
            fn XmfWebMMuxer_Free(ctx: *mut XmfWebMMuxer) -> (),

            // Image
            fn XmfImage_LoadFile(filename: *const c_char, data: *mut *mut u8, width: *mut u32, height: *mut u32, step: *mut u32) -> bool,
            fn XmfImage_SaveFile(filename: *const c_char, data: *const u8, width: u32, height: u32, step: u32) -> bool,
            fn XmfImage_FreeData(data: *mut u8) -> (),

            // VPX Encoder
            fn XmfVpxEncoder_Create(config: XmfVpxEncoderConfig) -> *mut XmfVpxEncoder,
            fn XmfVpxEncoder_EncodeFrame(ctx: *mut XmfVpxEncoder, image: *const XmfVpxImage, pts: i64, duration: usize, flags: u32) -> c_int,
            fn XmfVpxEncoder_GetEncodedFrame(ctx: *mut XmfVpxEncoder, output: *mut *mut u8, output_size: *mut usize) -> c_int,
            fn XmfVpxEncoder_Flush(ctx: *mut XmfVpxEncoder) -> c_int,
            fn XmfVpxEncoder_Destroy(ctx: *mut XmfVpxEncoder) -> c_int,
            fn XmfVpxEncoder_GetLastError(ctx: *const XmfVpxEncoder) -> XmfVpxEncoderError,
            fn XmfVpxEncoder_FreeEncodedFrame(input :*mut u8) -> (),
            fn XmfVpxEncoder_GetPacket(ctx: *mut XmfVpxEncoder, itr:*mut VpxIterator) -> *mut XmfVpxPacket,

            // VPX Decoder
            fn XmfVpxDecoder_Create(cfg: XmfVpxDecoderConfig) -> *mut XmfVpxDecoder,
            fn XmfVpxDecoder_Decode(ctx: *mut XmfVpxDecoder, data: *const u8, size: u32) -> c_int,
            fn XmfVpxDecoder_GetNextFrame(ctx: *mut XmfVpxDecoder) -> *mut XmfVpxImage,
            fn XmfVpxDecoder_GetLastError(ctx: *mut XmfVpxDecoder) -> XmfVpxDecoderError,
            fn XmfVpxDecoder_Destroy(ctx: *mut XmfVpxDecoder) -> (),

            // VPX Image
            fn XmfVpxImage_Destroy(ctx: *mut XmfVpxImage) -> (),

            // VPX Packet
            fn XmfVpxPacket_Destroy(packet: *mut XmfVpxPacket) -> (),
            fn XmfVpxPacket_GetKind(packet: *mut XmfVpxPacket) -> XmfVpxPacketKind,
            fn XmfVpxPacket_GetFrame(packet: *const XmfVpxPacket) -> *mut XmfVpxFrame,
            fn XmfVpxPacket_IsEmpty(packet: *const XmfVpxPacket) -> bool,

            // VPX Frame
            fn XmfVpxFrame_Destroy(frame: *mut XmfVpxFrame) -> (),
            fn XmfVpxFrame_GetSize(frame: *const XmfVpxFrame) -> usize,
            fn XmfVpxFrame_GetPts(frame: *const XmfVpxFrame) -> i64,
            fn XmfVpxFrame_GetDuration(frame: *const XmfVpxFrame) -> u64,
            fn XmfVpxFrame_GetFlags(frame: *const XmfVpxFrame) -> u32,
            fn XmfVpxFrame_GetPartitionId(frame: *const XmfVpxFrame) -> c_int,
            fn XmfVpxFrame_GetWidth(frame: *const XmfVpxFrame, layer: c_int) -> u32,
            fn XmfVpxFrame_GetHeight(frame: *const XmfVpxFrame, layer: c_int) -> u32,
            fn XmfVpxFrame_GetSpatialLayerEncoded(frame: *const XmfVpxFrame, layer: c_int) -> u8,
            fn XmfVpxFrame_GetBuffer(frame: *const XmfVpxFrame, buffer: *mut *const u8, size: *mut usize) -> c_int,

    );
}
