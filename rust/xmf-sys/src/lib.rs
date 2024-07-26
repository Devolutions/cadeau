#![doc = include_str!("../README.md")]
#![allow(clippy::too_many_arguments)]
#![allow(non_snake_case)]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]

mod macros;

use std::ffi::{c_char, c_int, c_void};

pub type XmfGetTimeFn = unsafe extern "C" fn(*mut c_void) -> u64;

#[repr(C)]
pub struct XmfTimeSource {
    pub func: XmfGetTimeFn,
    pub param: *mut c_void,
}

pub type XmfBipBuffer = c_void;

pub type XmfWebM = c_void;

pub type XmfRecorder = c_void;

pub type XmfWebMMuxer = c_void;

pub const XMF_MUXER_FILE_OPEN_ERROR: c_int = -1001;
pub const XMF_MUXER_PARSER_ERROR: c_int = -1002;
pub const XMF_MUXER_MUXER_ERROR: c_int = -1003;

#[doc(inline)]
pub use raw::global::*;

pub mod raw {
    use super::*;

    crate::macros::external_library!(feature = "dlopen", Api, "xmf",
        functions:
            // Time
            fn XmfTime_GetTickCount() -> u64,
            fn XmfTimeSource_Get(ts: *mut XmfTimeSource) -> u64,
            fn XmfTimeSource_System(param: *mut c_void) -> u64,
            fn XmfTimeSource_Manual(param: *mut c_void) -> u64,

            // BipBuffer
            fn XmfBipBuffer_Grow(ctx: *mut XmfBipBuffer, size: usize) -> bool,
            fn XmfBipBuffer_Clear(ctx: *mut XmfBipBuffer) -> (),
            fn XmfBipBuffer_UsedSize(ctx: *mut XmfBipBuffer) -> usize,
            fn XmfBipBuffer_BufferSize(ctx: *mut XmfBipBuffer) -> usize,
            fn XmfBipBuffer_Read(ctx: *mut XmfBipBuffer, data: *mut u8, size: usize) -> c_int,
            fn XmfBipBuffer_Write(ctx: *mut XmfBipBuffer, data: *const u8, size: usize) -> c_int,
            fn XmfBipBuffer_New(size: usize) -> *mut XmfBipBuffer,
            fn XmfBipBuffer_Free(ctx: *mut XmfBipBuffer) -> (),

            // WebM
            fn XmfWebM_Encode(ctx: *mut XmfWebM, srcData: *const u8, x: u16, y: u16, width: u16, height: u16) -> c_int,
            fn XmfWebM_Finalize(ctx: *mut XmfWebM) -> (),
            fn XmfWebM_FrameCount(ctx: *mut XmfWebM) -> u64,
            fn XmfWebM_Duration(ctx: *mut XmfWebM) -> u64,
            fn XmfWebM_Init(ctx: *mut XmfWebM, frameWidth: u32, frameHeight: u32, frameRate: u32, targetBitRate: u32, filename: *const c_char, bb: *mut XmfBipBuffer, ts: *mut XmfTimeSource) -> bool,
            fn XmfWebM_Uninit(ctx: *mut XmfWebM) -> (),
            fn XmfWebM_New() -> *mut XmfWebM,
            fn XmfWebM_Free(ctx: *mut XmfWebM) -> (),

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
    );
}
