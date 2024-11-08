use std::env;

use cadeau::xmf::vpx::{
    decoder::{VpxDecoder, VpxDecoderConfig},
    encoder::{VpxEncoder, VpxEncoderConfig},
    is_key_frame, VpxCodec,
};
use webm_iterable::{
    matroska_spec::{Block, Master, MatroskaSpec},
    WebmIterator,
};
use xmf_sys::vpx::VPX_ENCODE_FORCE_KEY_FRAME;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();
    let args: Args = args.try_into().map_err(|e| format!("error: {}", e))?;

    let file = std::fs::File::open(&args.input_path)?;
    let mut webm_iterator = WebmIterator::new(file, &[]);

    let mut codec = VpxCodec::VP8;
    let mut height = 0;
    let mut width = 0;
    while let Some(Ok(tag)) = webm_iterator.next() {
        if let MatroskaSpec::CodecID(ref codec_id) = tag {
            codec = if codec_id == "V_VP8" {
                VpxCodec::VP8
            } else {
                VpxCodec::VP9
            };
        }

        if let MatroskaSpec::PixelWidth(w) = tag {
            width = w;
        }

        if let MatroskaSpec::PixelHeight(h) = tag {
            height = h;
        }

        if let MatroskaSpec::Cluster(Master::Start) = tag {
            break;
        }
    }

    // SAFETY: Just pray at this point.
    #[cfg(feature = "dlopen")]
    unsafe {
        cadeau::xmf::init(
            args.lib_xmf_path
                .as_ref()
                .expect("xmf dll path is needed for dynamic loading"),
        )?;
    }

    let decoder_config = VpxDecoderConfig::builder()
        .codec(codec)
        .w(width as u32)
        .h(height as u32)
        .threads(3)
        .build();

    let encoder_config = VpxEncoderConfig::builder()
        .codec(codec)
        .threads(3)
        .width(width as u32)
        .height(height as u32)
        .timebase_num(1)
        .timebase_den(1000) // 1/1000 timebase
        .bitrate(1000)
        .build();

    println!(
        "decoder config built, width: {}, height: {}, codec {:?}",
        width, height, codec
    );

    let mut decoder = VpxDecoder::new(decoder_config);
    let mut encoder = VpxEncoder::new(encoder_config);

    println!("decoder initialized");
    let mut counter = 100;
    let mut pts = 0;
    let mut previous_block_holder: Option<Vec<u8>> = None;
    while let Some(tag) = webm_iterator.next() {
        let tag = tag?;
        if counter == 0 {
            break;
        }
        if let MatroskaSpec::Block(block_buffer) = tag {
            let block = Block::try_from(&block_buffer)?;
            let previous_block = match previous_block_holder.take() {
                Some(block_buffer) => block_buffer,
                None => {
                    previous_block_holder = Some(block_buffer.clone());
                    continue;
                }
            };

            let previous_block = Block::try_from(&previous_block)?;

            let duration = block.timestamp - previous_block.timestamp;

            let frame = previous_block.read_frame_data()?;
            for frame in frame {
                decoder.decode(frame.data).map_err(|e| format!("error: {:?}", e.code))?;
                counter -= 1;
                let Ok(image) = decoder.next_frame() else {
                    println!("error decoding frame");
                    continue;
                };

                encoder
                    .encode_frame(&image, pts as i64, duration as i64, VPX_ENCODE_FORCE_KEY_FRAME)
                    .map_err(|e| format!("error: {:?}", e.code))?;

                let Ok(encoded_frame) = encoder.next_frame() else {
                    println!("error encoding frame");
                    continue;
                };

                let is_keyframe = is_key_frame(&encoded_frame);

                println!("frame encoded successfully, keyframe: {}", is_keyframe);
            }

            previous_block_holder = Some(block_buffer.clone());
            pts += duration;
        }
    }

    loop {
        match decoder.next_frame() {
            Ok(_image) => {
                println!("frame decoded");
            }
            Err(e) => {
                println!("error decoding frame: {:?}", e.code);
                break;
            }
        }
    }

    println!("Decoding finished, counter: {}", counter);

    Ok(())
}

struct Args {
    // input path, -i
    input_path: String,
    // lib_xmf path, --lib-xmf
    lib_xmf_path: Option<String>,
    // output path, -o
    output_path: Option<String>,
}

impl TryFrom<Vec<String>> for Args {
    type Error = &'static str;

    fn try_from(value: Vec<String>) -> Result<Self, Self::Error> {
        // search for -i
        let input_path = value
            .iter()
            .enumerate()
            .find(|(_, v)| *v == "-i")
            .filter(|(i, _)| i + 1 < value.len())
            .map(|(i, _)| value[i + 1].clone())
            .ok_or("missing input path")?;

        let lib_xmf_path = value
            .iter()
            .enumerate()
            .find(|(_, v)| *v == "--lib-xmf")
            .filter(|(i, _)| i + 1 < value.len())
            .map(|(i, _)| value[i + 1].clone());

        let output_path = value
            .iter()
            .enumerate()
            .find(|(_, v)| *v == "-o")
            .filter(|(i, _)| i + 1 < value.len())
            .map(|(i, _)| value[i + 1].clone());

        Ok(Self {
            input_path,
            lib_xmf_path,
            output_path,
        })
    }
}
