use core::mem::MaybeUninit;

use super::VpxPlane;

#[test]
fn rows_skip_uninitialized_padding() {
    let mut data = [MaybeUninit::uninit(); 11];
    for (start, pixels) in [(0, [1, 2, 3]), (4, [4, 5, 6]), (8, [7, 8, 9])] {
        for (offset, value) in pixels.into_iter().enumerate() {
            data[start + offset].write(value);
        }
    }
    let plane = VpxPlane {
        data: &data,
        columns: 3,
        stride: 4,
    };

    assert_eq!(plane.stride(), 4);
    assert_eq!((plane.width(), plane.height()), (3, 3));
    assert_eq!(plane.rows().len(), 3);
    let rows: Vec<_> = plane.rows().collect();
    assert_eq!(rows, [&[1, 2, 3][..], &[4, 5, 6][..], &[7, 8, 9][..]]);
    assert_eq!(rows[0].as_ptr(), data.as_ptr().cast());
    assert_eq!(rows[1].as_ptr(), data[4..].as_ptr().cast());
}

#[test]
fn as_ptr_reaches_every_row_at_the_stride() {
    let mut data = [MaybeUninit::uninit(); 11];
    for (start, pixels) in [(0, [1, 2, 3]), (4, [4, 5, 6]), (8, [7, 8, 9])] {
        for (offset, value) in pixels.into_iter().enumerate() {
            data[start + offset].write(value);
        }
    }
    let plane = VpxPlane {
        data: &data,
        columns: 3,
        stride: 4,
    };

    // SAFETY: Only the initialized pixels of each row are read, while `data` is alive.
    let base = unsafe { plane.as_ptr() };
    for (index, row) in plane.rows().enumerate() {
        // SAFETY: Row `index` starts `index * stride` bytes after `base`, inside the plane.
        let start = unsafe { base.add(index * plane.stride()) };
        // SAFETY: The row has `width` initialized pixels.
        let pixels = unsafe { core::slice::from_raw_parts(start, plane.width()) };
        assert_eq!(pixels, row);
    }
}

#[test]
fn debug_prints_geometry_not_pixels() {
    let data = [MaybeUninit::new(7); 11];
    let plane = VpxPlane {
        data: &data,
        columns: 3,
        stride: 4,
    };

    assert_eq!(format!("{plane:?}"), "VpxPlane { width: 3, height: 3, stride: 4, .. }");
}

#[test]
fn rows_allow_a_single_row_without_trailing_padding() {
    let data = [MaybeUninit::new(42); 3];
    let plane = VpxPlane {
        data: &data,
        columns: 3,
        stride: 8,
    };

    assert_eq!(plane.rows().len(), 1);
    assert_eq!(plane.rows().next(), Some(&[42, 42, 42][..]));
}

#[test]
fn rows_allow_tightly_packed_pixels() {
    let data = [MaybeUninit::new(42); 6];
    let plane = VpxPlane {
        data: &data,
        columns: 3,
        stride: 3,
    };

    assert_eq!(plane.rows().len(), 2);
    assert!(plane.rows().all(|row| row == [42, 42, 42]));
}

/// A VP8 key frame of a flat red 321x241 image.
#[cfg(not(feature = "dlopen"))]
const RED_321X241_VP8_KEY_FRAME: &[u8] = &[
    0xf0, 0x14, 0x00, 0x9d, 0x01, 0x2a, 0x41, 0x01, 0xf1, 0x00, 0x00, 0x47, 0x08, 0x85, 0x85, 0x88, 0x85, 0x84, 0x88,
    0x02, 0x02, 0x02, 0x75, 0xaa, 0x03, 0xf8, 0x03, 0xfa, 0x02, 0x06, 0xb6, 0xa4, 0xf7, 0x06, 0x81, 0x64, 0x9f, 0x6b,
    0xdb, 0x9b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38,
    0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b,
    0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27,
    0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38,
    0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b,
    0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27,
    0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38, 0x7b, 0x27, 0x38,
    0x7b, 0x27, 0x38, 0x7b, 0x22, 0x80, 0xfe, 0xfd, 0x6e, 0xf3, 0xff, 0xe3, 0x99, 0x37, 0x30, 0xc4, 0xff, 0x8e, 0x6d,
    0xff, 0xf1, 0x61, 0x3c, 0x0e, 0x28, 0xc8, 0xff, 0xf1, 0x51, 0x00,
];

#[cfg(not(feature = "dlopen"))]
#[test]
fn decoded_odd_sized_vp8_rows_contain_only_pixels() {
    use super::{VpxColorRange, VpxColorSpace, VpxDecoder, VpxImageFormat};

    let mut decoder = VpxDecoder::builder().threads(1).build().expect("create decoder");
    decoder
        .decode(RED_321X241_VP8_KEY_FRAME)
        .expect("decode synthetic red frame");
    let image = decoder.next_frame().expect("decoded image");
    assert_eq!((image.width(), image.height()), (321, 241));
    assert_eq!(image.format(), VpxImageFormat::I420);
    assert_eq!(image.color_space(), VpxColorSpace::UNKNOWN);
    assert_eq!(image.color_range(), VpxColorRange::STUDIO);

    let planes = image.i420_planes().expect("I420 planes");
    assert!(planes.y.stride() > 321);
    for (plane, width, height, value) in [
        (planes.y, 321, 241, 81),
        (planes.u, 161, 121, 90),
        (planes.v, 161, 121, 240),
    ] {
        assert_eq!((plane.width(), plane.height()), (width, height));
        assert_eq!(plane.rows().len(), height);
        for row in plane.rows() {
            assert_eq!(row.len(), width);
            assert!(row.iter().all(|&pixel| pixel == value));
        }

        // SAFETY: Only pixel bytes are read, while `image` keeps the decoder borrowed.
        let base = unsafe { plane.as_ptr() };
        for (index, row) in plane.rows().enumerate() {
            // SAFETY: Row `index` starts `index * stride` bytes after `base`, inside the plane.
            let start = unsafe { base.add(index * plane.stride()) };
            // SAFETY: The row has `width` initialized pixels.
            let pixels = unsafe { core::slice::from_raw_parts(start, plane.width()) };
            assert_eq!(pixels, row);
        }
    }
}

#[cfg(not(feature = "dlopen"))]
fn test_encoder(width: u32, height: u32) -> super::VpxEncoderBuilder {
    super::VpxEncoder::builder()
        .width(width)
        .height(height)
        .bitrate(100)
        .timebase_num(1)
        .timebase_den(1000)
        .threads(1)
}

#[cfg(not(feature = "dlopen"))]
#[test]
fn encoder_with_quantizer_range_encodes_a_key_frame() {
    use super::{is_key_frame, VpxDecoder};

    let mut decoder = VpxDecoder::builder().threads(1).build().expect("create decoder");
    decoder
        .decode(RED_321X241_VP8_KEY_FRAME)
        .expect("decode synthetic red frame");
    let image = decoder.next_frame().expect("decoded image");

    let mut encoder = test_encoder(321, 241)
        .min_quantizer(4)
        .max_quantizer(30)
        .build()
        .expect("create encoder");
    encoder.encode_frame(&image, 0, 1, 0).expect("encode frame");
    let frame = encoder.next_frame().expect("get encoded frame").expect("encoded frame");
    assert!(is_key_frame(&frame));
}

#[cfg(not(feature = "dlopen"))]
#[test]
fn encoder_rejects_invalid_quantizer_range() {
    use xmf_sys::{XmfVpxEncoderError, XmfVpxEncoderErrorCode};

    use super::VpxError;

    for (min, max) in [
        (None, Some(64)),
        (Some(64), None),
        (Some(40), Some(30)),
        // libvpx's VP8 default min quantizer is 4.
        (None, Some(2)),
    ] {
        let mut builder = test_encoder(321, 241);
        if let Some(min) = min {
            builder = builder.min_quantizer(min);
        }
        if let Some(max) = max {
            builder = builder.max_quantizer(max);
        }

        let result = builder.build();
        assert!(
            matches!(
                result,
                Err(VpxError::EncoderError(XmfVpxEncoderError {
                    code: XmfVpxEncoderErrorCode::InvalidParam,
                    ..
                }))
            ),
            "min {min:?}, max {max:?} should be rejected"
        );
    }
}

#[cfg(not(feature = "dlopen"))]
#[test]
fn encoder_with_default_quantizers_matches_encoder_without_them() {
    use super::VpxDecoder;

    let mut decoder = VpxDecoder::builder().threads(1).build().expect("create decoder");
    decoder
        .decode(RED_321X241_VP8_KEY_FRAME)
        .expect("decode synthetic red frame");
    let image = decoder.next_frame().expect("decoded image");

    let encode = |builder: super::VpxEncoderBuilder| {
        let mut encoder = builder.build().expect("create encoder");
        encoder.encode_frame(&image, 0, 1, 0).expect("encode frame");
        encoder.next_frame().expect("get encoded frame").expect("encoded frame")
    };

    // libvpx's VP8 defaults, so only the XmfVpxEncoder_CreateEx path differs.
    let explicit = encode(test_encoder(321, 241).min_quantizer(4).max_quantizer(63));
    let implicit = encode(test_encoder(321, 241));
    assert_eq!(explicit, implicit);
}

/// A 48x48 VP8 key frame of FFmpeg's `testsrc2` pattern: enough detail for the quantizer to change the encoded size.
#[cfg(not(feature = "dlopen"))]
const PATTERN_48X48_VP8_KEY_FRAME: &[u8] = &[
    0xb0, 0x08, 0x00, 0x9d, 0x01, 0x2a, 0x30, 0x00, 0x30, 0x00, 0x03, 0x87, 0x08, 0x85, 0x85, 0x88, 0x99, 0x84, 0x88,
    0x35, 0x02, 0x02, 0x75, 0xd5, 0x0b, 0x44, 0xf9, 0xb1, 0x98, 0x38, 0x78, 0xa4, 0xdf, 0x9e, 0x29, 0xe7, 0xeb, 0x0d,
    0xe2, 0xff, 0xab, 0xc5, 0xc7, 0x87, 0x83, 0xd6, 0x50, 0x88, 0x2d, 0xf1, 0x6c, 0x0f, 0x1e, 0x96, 0x90, 0xe0, 0x39,
    0xd7, 0xd6, 0x0d, 0x59, 0x41, 0xf1, 0x8f, 0xe2, 0xd4, 0xd2, 0x3d, 0x07, 0xbd, 0x00, 0x8d, 0x1e, 0xe8, 0xdc, 0xae,
    0xe8, 0xba, 0x78, 0xfc, 0x8f, 0x27, 0xd1, 0x28, 0x65, 0x17, 0x8c, 0x53, 0x5c, 0x81, 0xf7, 0x34, 0x36, 0x58, 0xf1,
    0x3f, 0xf1, 0xde, 0x15, 0x5c, 0xc4, 0x96, 0xa0, 0xa5, 0x32, 0x0e, 0x3e, 0x43, 0x4e, 0x5c, 0xc1, 0x1f, 0x73, 0xd2,
    0xad, 0x41, 0xc0, 0xdc, 0x48, 0xcc, 0x46, 0x02, 0x55, 0x90, 0x8e, 0xf9, 0xe9, 0x83, 0x07, 0x85, 0x01, 0xce, 0xbd,
    0xff, 0x15, 0x95, 0x4f, 0x61, 0xaf, 0x25, 0x0b, 0x8a, 0x94, 0x42, 0xbf, 0x35, 0xaa, 0x32, 0x98, 0x5e, 0xf6, 0x54,
    0xd7, 0xcb, 0x8d, 0x96, 0xe0, 0xdb, 0x79, 0xc3, 0xbc, 0x8c, 0x33, 0x26, 0x19, 0xc5, 0xe7, 0xb0, 0xd6, 0xee, 0x1e,
    0xbb, 0xaa, 0x6b, 0x0a, 0x4d, 0xa0, 0xcd, 0x06, 0x42, 0x91, 0x73, 0x47, 0x27, 0x48, 0x10, 0xc5, 0xc1, 0x69, 0x08,
    0xa8, 0x8b, 0xed, 0xb8, 0x1d, 0x00, 0x00, 0x02, 0x12, 0xdc, 0xae, 0x4b, 0x77, 0x12, 0x69, 0xb6, 0x75, 0x48, 0x00,
    0x00, 0x01, 0x70, 0xa0, 0x20, 0x00, 0x00, 0x00, 0xff, 0x45, 0xa8, 0x04, 0xb1, 0x0d, 0x24, 0xb9, 0x80, 0x13, 0x5e,
    0xbf, 0x42, 0x00, 0x00, 0xef, 0x4e, 0x5d, 0xe1, 0x8f, 0x1c, 0x47, 0x07, 0x6e, 0x6b, 0xd6, 0xe0, 0x3a, 0x00, 0x85,
    0xcc, 0x5a, 0x36, 0xee, 0x00, 0x6c, 0x40, 0x0b, 0x81, 0xcb, 0x7d, 0xfe, 0x78, 0x00, 0xb4, 0xc6, 0xf2, 0x48, 0xca,
    0x2f, 0x13, 0x20,
];

#[cfg(not(feature = "dlopen"))]
#[test]
fn encoder_applies_quantizer_range() {
    use super::VpxDecoder;

    let mut decoder = VpxDecoder::builder().threads(1).build().expect("create decoder");
    decoder
        .decode(PATTERN_48X48_VP8_KEY_FRAME)
        .expect("decode pattern frame");
    let image = decoder.next_frame().expect("decoded image");
    assert_eq!((image.width(), image.height()), (48, 48));

    let encoded_size = |builder: super::VpxEncoderBuilder| {
        let mut encoder = builder.bitrate(1).build().expect("create encoder");
        encoder.encode_frame(&image, 0, 1, 0).expect("encode frame");
        encoder
            .next_frame()
            .expect("get encoded frame")
            .expect("encoded frame")
            .len()
    };

    // Same frame and settings, so only the range changes the size: a low cap forces finer, larger frames,
    // a high floor forces coarser, smaller ones.
    let unbounded = encoded_size(test_encoder(48, 48));
    let capped = encoded_size(test_encoder(48, 48).max_quantizer(10));
    let floored = encoded_size(test_encoder(48, 48).min_quantizer(50));
    assert!(
        capped > unbounded,
        "max quantizer 10: {capped} bytes, unbounded: {unbounded} bytes"
    );
    assert!(
        floored < unbounded,
        "min quantizer 50: {floored} bytes, unbounded: {unbounded} bytes"
    );
}
