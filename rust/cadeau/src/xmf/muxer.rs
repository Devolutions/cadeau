use core::fmt;
use std::ffi::CString;
use std::path::Path;

use xmf_sys::{
    XmfWebMMuxer, XmfWebMMuxer_Free, XmfWebMMuxer_New, XmfWebMMuxer_Remux, XMF_MUXER_FILE_OPEN_ERROR,
    XMF_MUXER_MUXER_ERROR, XMF_MUXER_PARSER_ERROR,
};

#[derive(Debug, Clone)]
pub enum MuxerError {
    BadArgument { name: &'static str },
    FileOpen,
    Parser,
    Muxer,
    Unexpected,
}

impl std::error::Error for MuxerError {}

impl fmt::Display for MuxerError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            MuxerError::BadArgument { name } => write!(f, "bad argument: {name}"),
            MuxerError::FileOpen => write!(f, "failed to open a file"),
            MuxerError::Parser => write!(f, "parser failed"),
            MuxerError::Muxer => write!(f, "muxer failed"),
            MuxerError::Unexpected => write!(f, "an unexpected error happened"),
        }
    }
}

/// Performs a remux operation on the WebM file found at `input_path` and writes the result into a new file at `output_path`
pub fn webm_remux(input_path: impl AsRef<Path>, output_path: impl AsRef<Path>) -> Result<(), MuxerError> {
    let mut webm_muxer = WebMMuxer::new();
    webm_muxer.remux(input_path.as_ref(), output_path.as_ref())
}

struct WebMMuxer {
    ptr: *mut XmfWebMMuxer,
}

impl WebMMuxer {
    fn new() -> Self {
        // SAFETY: FFI call with no outstanding precondition.
        let inner = unsafe { XmfWebMMuxer_New() };

        Self { ptr: inner }
    }

    fn remux(&mut self, input_path: &Path, output_path: &Path) -> Result<(), MuxerError> {
        let input_path = input_path
            .to_str()
            .and_then(|s| CString::new(s).ok())
            .ok_or(MuxerError::BadArgument { name: "input_path" })?;

        let output_path = output_path
            .to_str()
            .and_then(|s| CString::new(s).ok())
            .ok_or(MuxerError::BadArgument { name: "output_path" })?;

        // SAFETY: We ensured that input_path and output_path are valid C strings.
        let ret = unsafe { XmfWebMMuxer_Remux(self.ptr, input_path.as_ptr(), output_path.as_ptr()) };

        match ret {
            XMF_MUXER_FILE_OPEN_ERROR => Err(MuxerError::FileOpen),
            XMF_MUXER_PARSER_ERROR => Err(MuxerError::Parser),
            XMF_MUXER_MUXER_ERROR => Err(MuxerError::Muxer),
            0 => Ok(()),
            _ => Err(MuxerError::Unexpected),
        }
    }
}

impl Drop for WebMMuxer {
    fn drop(&mut self) {
        // SAFETY: The pointer is owned.
        unsafe { XmfWebMMuxer_Free(self.ptr) };
    }
}

impl Default for WebMMuxer {
    fn default() -> Self {
        Self::new()
    }
}
