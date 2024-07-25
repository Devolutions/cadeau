use core::fmt;
use std::ffi::CString;
use std::path::Path;

use cadeau_sys::xmf::{
    XmfRecorder, XmfRecorder_Free, XmfRecorder_GetCurrentTime, XmfRecorder_GetFrameRate, XmfRecorder_GetTimeout,
    XmfRecorder_Init, XmfRecorder_New, XmfRecorder_SetBipBuffer, XmfRecorder_SetCurrentTime, XmfRecorder_SetFileName,
    XmfRecorder_SetFrameRate, XmfRecorder_SetFrameSize, XmfRecorder_SetMinimumFrameRate, XmfRecorder_SetVideoQuality,
    XmfRecorder_Timeout, XmfRecorder_Uninit, XmfRecorder_UpdateFrame,
};

use crate::xmf::bip_buffer::BipBuffer;

#[derive(Debug, Clone)]
pub enum RecorderError {
    BadArgument { name: &'static str },
    InitFailed,
}

impl std::error::Error for RecorderError {}

impl fmt::Display for RecorderError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            RecorderError::BadArgument { name } => write!(f, "bad argument: {name}"),
            RecorderError::InitFailed => write!(f, "failed to initialize the recorder"),
        }
    }
}

pub struct RecorderBuilder {
    frame_width: usize,
    frame_height: usize,
    frame_rate: u32,
    video_quality: Option<u32>,
    minimum_frame_rate: Option<u32>,
    current_time: Option<u64>,
    bip_buffer: Option<BipBuffer>,
}

pub struct Recorder {
    ptr: *mut XmfRecorder,
}

impl RecorderBuilder {
    pub fn new(frame_width: usize, frame_height: usize) -> Self {
        Self {
            frame_width,
            frame_height,
            frame_rate: 10,
            video_quality: None,
            minimum_frame_rate: None,
            current_time: None,
            bip_buffer: None,
        }
    }

    #[must_use]
    pub fn frame_rate(mut self, frame_rate: u32) -> Self {
        self.frame_rate = frame_rate;
        self
    }

    #[must_use]
    pub fn video_quality(mut self, video_quality: u32) -> Self {
        self.video_quality = Some(video_quality);
        self
    }

    #[must_use]
    pub fn minimum_frame_rate(mut self, frame_rate: u32) -> Self {
        self.minimum_frame_rate = Some(frame_rate);
        self
    }

    #[must_use]
    pub fn current_time(mut self, current_time: u64) -> Self {
        self.current_time = Some(current_time);
        self
    }

    #[must_use]
    pub fn bip_buffer(mut self, bip_buffer: BipBuffer) -> Self {
        self.bip_buffer = Some(bip_buffer);
        self
    }

    pub fn init(self, filename: impl AsRef<Path>) -> Result<Recorder, RecorderError> {
        self.init_impl(filename.as_ref())
    }

    fn init_impl(self, filename: &Path) -> Result<Recorder, RecorderError> {
        let mut recorder = Recorder::new();

        recorder.set_filename(filename)?;
        recorder.set_frame_size(self.frame_width, self.frame_height)?;
        recorder.set_frame_rate(self.frame_rate);

        if let Some(video_quality) = self.video_quality {
            recorder.set_video_quality(video_quality);
        }

        if let Some(minimum_frame_rate) = self.minimum_frame_rate {
            recorder.set_minimum_frame_rate(minimum_frame_rate);
        }

        if let Some(current_time) = self.current_time {
            recorder.set_current_time(current_time);
        }

        if let Some(bip_buffer) = self.bip_buffer {
            recorder.set_bip_buffer(bip_buffer);
        }

        recorder.init()?;

        Ok(recorder)
    }
}

impl Recorder {
    pub fn builder(frame_width: usize, frame_height: usize) -> RecorderBuilder {
        RecorderBuilder::new(frame_width, frame_height)
    }

    pub fn update_frame(
        &mut self,
        buffer: &[u8],
        x: usize,
        y: usize,
        width: usize,
        height: usize,
        surface_step: usize,
    ) -> Result<(), RecorderError> {
        if buffer.len() < surface_step * height {
            return Err(RecorderError::BadArgument { name: "buffer" });
        }

        // Currently Cadeau is hardcoding this pixel depth instead of using surface_step, so we check for that too.
        // This check should be removed if Cadeau is modified to use surface_step.
        if buffer.len() < width * height * 4 {
            return Err(RecorderError::BadArgument { name: "buffer" });
        }

        let x = u32::try_from(x).map_err(|_| RecorderError::BadArgument { name: "x" })?;
        let y = u32::try_from(y).map_err(|_| RecorderError::BadArgument { name: "y" })?;
        let width = u32::try_from(width).map_err(|_| RecorderError::BadArgument { name: "width" })?;
        let height = u32::try_from(height).map_err(|_| RecorderError::BadArgument { name: "height" })?;
        let surface_step =
            u32::try_from(surface_step).map_err(|_| RecorderError::BadArgument { name: "surface_step" })?;

        // SAFETY: We ensured that buffer was big enough for the given width and height.
        unsafe { XmfRecorder_UpdateFrame(self.ptr, buffer.as_ptr(), x, y, width, height, surface_step) };

        Ok(())
    }

    #[inline]
    pub fn get_timeout(&mut self) -> u32 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_GetTimeout(self.ptr) }
    }

    #[inline]
    pub fn timeout(&mut self) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_Timeout(self.ptr) }
    }

    #[inline]
    pub fn get_frame_rate(&mut self) -> u32 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_GetFrameRate(self.ptr) }
    }

    #[inline]
    pub fn set_video_quality(&mut self, video_quality: u32) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_SetVideoQuality(self.ptr, video_quality) }
    }

    #[inline]
    pub fn set_current_time(&mut self, current_time: u64) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_SetCurrentTime(self.ptr, current_time) }
    }

    #[inline]
    pub fn get_current_time(&mut self) -> u64 {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_GetCurrentTime(self.ptr) }
    }

    /// Returns a raw pointer to the underlying FFI handle.
    #[inline]
    pub const fn as_ptr(&self) -> *mut XmfRecorder {
        self.ptr
    }

    // == Hidden from the idiomatic API == //

    fn new() -> Self {
        // SAFETY: FFI call with no outstanding precondition.
        let ptr = unsafe { XmfRecorder_New() };

        Self { ptr }
    }

    fn set_filename(&mut self, filename: &Path) -> Result<(), RecorderError> {
        let filename = filename
            .to_str()
            .and_then(|s| CString::new(s).ok())
            .ok_or(RecorderError::BadArgument { name: "filename" })?;

        // SAFETY: We ensured that filename is a valid C string.
        unsafe { XmfRecorder_SetFileName(self.ptr, filename.as_ptr()) };

        Ok(())
    }

    fn set_frame_size(&mut self, frame_width: usize, frame_height: usize) -> Result<(), RecorderError> {
        let frame_width = u32::try_from(frame_width).map_err(|_| RecorderError::BadArgument { name: "frame_width" })?;
        let frame_height =
            u32::try_from(frame_height).map_err(|_| RecorderError::BadArgument { name: "frame_height" })?;

        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_SetFrameSize(self.ptr, frame_width, frame_height) };

        Ok(())
    }

    fn set_frame_rate(&mut self, frame_rate: u32) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_SetFrameRate(self.ptr, frame_rate) };
    }

    fn set_minimum_frame_rate(&mut self, frame_rate: u32) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_SetMinimumFrameRate(self.ptr, frame_rate) }
    }

    fn set_bip_buffer(&mut self, bip_buffer: BipBuffer) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_SetBipBuffer(self.ptr, bip_buffer.as_ptr()) }
    }

    fn init(&mut self) -> Result<(), RecorderError> {
        // SAFETY: FFI call with no outstanding precondition.
        let ret = unsafe { XmfRecorder_Init(self.ptr) };

        if ret {
            Ok(())
        } else {
            Err(RecorderError::InitFailed)
        }
    }

    fn uninit(&mut self) {
        // SAFETY: FFI call with no outstanding precondition.
        unsafe { XmfRecorder_Uninit(self.ptr) }
    }
}

impl Drop for Recorder {
    fn drop(&mut self) {
        self.uninit();

        // SAFETY: The pointer is owned.
        unsafe { XmfRecorder_Free(self.ptr) };
    }
}
