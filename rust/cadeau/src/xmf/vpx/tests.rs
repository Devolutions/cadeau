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
fn red_frame_encoder() -> super::VpxEncoderBuilder {
    super::VpxEncoder::builder()
        .width(321)
        .height(241)
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

    let mut encoder = red_frame_encoder()
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
        let mut builder = red_frame_encoder();
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
    let explicit = encode(red_frame_encoder().min_quantizer(4).max_quantizer(63));
    let implicit = encode(red_frame_encoder());
    assert_eq!(explicit, implicit);
}
