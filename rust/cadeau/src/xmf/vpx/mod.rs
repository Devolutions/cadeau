use xmf_sys::{vpx::XmfVpxCodecType, XmfVpxImage, XmfVpxImage_Destroy, XmfVpxImage_GetData};

pub mod decoder;
pub mod encoder;

#[derive(Debug, Clone, Copy)]
pub enum VpxCodec {
    VP8,
    VP9,
}

pub struct VpxImage {
    pub(crate) ptr: *const XmfVpxImage,
}

impl VpxImage {
    pub fn is_empty(&self) -> bool {
        unsafe { XmfVpxImage_GetData(self.ptr as *mut XmfVpxImage).is_null() }
    }
}

impl Drop for VpxImage {
    fn drop(&mut self) {
        unsafe {
            XmfVpxImage_Destroy(self.ptr as *mut XmfVpxImage);
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
