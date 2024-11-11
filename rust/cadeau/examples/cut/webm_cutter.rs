use std::{io::Seek, path::Path};

use cadeau::xmf::vpx::{
    decoder::{VpxDecoder, VpxDecoderConfig},
    encoder::{VpxEncoder, VpxEncoderConfig},
    is_key_frame, VpxCodec, VpxImage,
};
use ebml_iterable::tools::Vint;
use webm_iterable::{
    matroska_spec::{Block, BlockLacing, Master, MatroskaSpec},
    WebmIterator,
};
use xmf_sys::vpx::VPX_EFLAG_FORCE_KF;

pub struct WebmCutter {
    encoder: VpxEncoder,
    decoder: VpxDecoder,
    iterator: Option<WebmIterator<std::fs::File>>,
    cut_time: u32,
    headers: Vec<MatroskaSpec>,
    time_stamp_scale: u64,
}

// Cutter for webm videos with only video tracks and no cues.
impl WebmCutter {
    pub fn new(input: impl AsRef<Path>, cut_time: u32) -> Result<Self, Box<dyn std::error::Error>> {
        let file = std::fs::File::open(&input)?;
        let mut webm_iterator = WebmIterator::new(file, &[MatroskaSpec::BlockGroup(Master::Start)]);
        let mut headers = Vec::new();
        let mut codec = VpxCodec::VP8;
        let mut height = 0;
        let mut width = 0;
        let mut time_stamp_scale = 0;
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

            if let MatroskaSpec::TimestampScale(ts) = tag {
                time_stamp_scale = ts;
            }

            if let MatroskaSpec::Cluster(Master::Start) = tag {
                break;
            }

            headers.push(tag);
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
            .timebase_num(1) // Was backwards - this should be denominator
            .timebase_den(1000) // Was backwards - this should be numerator
            .bitrate(256 * 1024) // Set to 256Kbps
            .build();

        let decoder = VpxDecoder::new(decoder_config);
        let encoder = VpxEncoder::new(encoder_config);

        Ok(Self {
            encoder,
            decoder,
            iterator: Some(webm_iterator),
            cut_time,
            headers,
            time_stamp_scale,
        })
    }

    pub fn on_element(&mut self, write: impl Fn(MatroskaSpec) -> ()) -> Result<(), Box<dyn std::error::Error>> {
        while self.headers.len() > 0 {
            let tag = self.headers.remove(0);
            write(tag);
        }
        println!("writing headers done");

        // 1. continue iterating until we find the block we want to cut, while keeping track of the last keyframe
        let mut last_keyframe_offset = None;
        let mut latest_custer_timestamp_read = None;
        let mut block_abs_time = 0;
        while let Some(Ok(tag)) = self.next_tag() {
            if let MatroskaSpec::Timestamp(ts) = tag {
                latest_custer_timestamp_read = Some(ts);
            }

            let MatroskaSpec::BlockGroup(Master::Full(_)) = &tag else {
                continue;
            };

            if is_key_frame_block_group(&tag) {
                last_keyframe_offset = Some(self.tag_offset());
            }

            let current_block_time = block_group_relative_time(&tag)?;

            if current_block_time as u64 + latest_custer_timestamp_read.unwrap() >= self.cut_time_in_ms() {
                block_abs_time = current_block_time + latest_custer_timestamp_read.unwrap() as i16;
                break;
            }
        }
        // 2. reseek to the last keyframe and start decoding from there until we hit the cut time block
        self.seek_iterator_to(
            last_keyframe_offset.unwrap() as u64 - 3, //3 bytes to offset the block group start
            &[MatroskaSpec::BlockGroup(Master::Start)],
        )?;

        let block_group = loop {
            // decode/encode the frame as we go (need to start from the keyframe)
            let Some(Ok(tag)) = self.next_tag() else {
                continue;
            };

            if let MatroskaSpec::Cluster(Master::End) = tag {
                unimplemented!("this case is not handled yet");
            }

            let MatroskaSpec::BlockGroup(Master::Full(_)) = &tag else {
                continue;
            };

            let current_block_time = block_group_relative_time(&tag)?;
            let current_block_abs_time = current_block_time + latest_custer_timestamp_read.unwrap() as i16;

            if current_block_abs_time >= self.cut_time_in_ms() as i16 {
                break tag;
            }

            let images = self.decode_block_group(&tag)?;

            for image in &images {
                let _ = self.encode(image, 0, 17, false)?;
            }
        };

        let mut pts: i16 = 0;
        let mut duration = 300; // 17ms
                                // 3.encode this frame (cut frame) as keyframe
                                // let block_group = self.

        // 4. continue iterating and encoding the rest of the frames until the end of the cluster

        // change the timestamp of every cluster after the cut time
        while let Some(Ok(tag)) = self.next_tag() {
            if let MatroskaSpec::Timestamp(ts) = tag {
                let new_ts = ts - block_abs_time as u64;
                write(MatroskaSpec::Timestamp(new_ts));
                continue;
            }
            write(tag);
        }

        Ok(())
    }

    fn cut_time_in_ms(&self) -> u64 {
        let cutime_in_nanoseconds = self.cut_time as u64 * 1_000_000_000;
        cutime_in_nanoseconds / self.time_stamp_scale
    }

    fn seek_iterator_to(
        &mut self,
        offset: u64,
        tags_to_buffer: &[MatroskaSpec],
    ) -> Result<(), Box<dyn std::error::Error>> {
        let iterator = self.iterator.take().unwrap();
        let mut file = iterator.into_inner();
        file.seek(std::io::SeekFrom::Start(offset))?;
        let webm_iterator = WebmIterator::new(file, tags_to_buffer);
        self.iterator = Some(webm_iterator);
        Ok(())
    }

    fn next_tag(&mut self) -> Option<Result<MatroskaSpec, Box<dyn std::error::Error>>> {
        let tag = match self.iterator.as_mut() {
            Some(iterator) => iterator.next(),
            None => return Some(Err("no iterator available".into())),
        };
        tag.map(|t| t.map_err(|e| e.into()))
    }

    fn tag_offset(&self) -> usize {
        self.iterator
            .as_ref()
            .map(|iterator| iterator.last_emitted_tag_offset())
            .unwrap()
    }

    fn decode_block_group(&mut self, block_group: &MatroskaSpec) -> Result<Vec<VpxImage>, Box<dyn std::error::Error>> {
        let MatroskaSpec::BlockGroup(Master::Full(block_group)) = block_group else {
            return Err("not a block group".into());
        };

        let block = block_group.into_iter().find_map(|tag| {
            if let MatroskaSpec::Block(block) = tag {
                Some(block)
            } else {
                None
            }
        });

        let Some(block) = block else {
            return Err("no block found in block group".into());
        };

        let block = Block::try_from(block).unwrap();

        let frame = block.read_frame_data()?;

        let mut images = Vec::new();

        for frame in frame {
            self.decoder
                .decode(frame.data)
                .map_err(|e| format!("error: {:?}", e.code))?;

            let image = self.decoder.next_frame().map_err(|e| format!("error: {:?}", e.code))?;
            images.push(image);
        }

        Ok(images)
    }

    fn encode(
        &mut self,
        image: &VpxImage,
        pts: i64,
        duration: i64,
        is_keyframe: bool,
    ) -> Result<Vec<u8>, Box<dyn std::error::Error>> {
        self.encoder
            .encode_frame(image, pts, duration, if is_keyframe { VPX_EFLAG_FORCE_KF } else { 0 })
            .map_err(|e| format!("error: {:?}", e.code))?;

        let frame = self
            .encoder
            .next_frame()
            .map_err(|e| format!("error: {:?}", e.code))?
            .ok_or("no frame")?;

        println!("encoded frame: {:?}", frame.len()); // debug

        Ok(frame)
    }

    pub fn re_encode_block_group(
        &mut self,
        block_group: &MatroskaSpec,
        offset: i16,
    ) -> Result<MatroskaSpec, Box<dyn std::error::Error>> {
        todo!()
        // let block_time = block_group_relative_time(block_group)?;
        // let block_time = block_time - offset;
        // let images = self.decode_block_group(block_group)?;

        // let mut encoded_frames = Vec::new();
        // for image in images {
        //     let encoded_frame = self.encode(&image, block_time, duration, true)?;
        //     encoded_frames.push(encoded_frame);
        // }

        // let block_group = BlockGroup::new(0, 0, &encoded_frames[0], true);
        // Ok(block_group.into())
    }
}

pub fn is_key_frame_block_group(block_group: &MatroskaSpec) -> bool {
    let MatroskaSpec::BlockGroup(Master::Full(block_group)) = block_group else {
        return false;
    };

    let block = block_group.into_iter().find_map(|tag| {
        if let MatroskaSpec::Block(block) = tag {
            Some(block)
        } else {
            None
        }
    });

    let Some(block) = block else {
        return false;
    };

    let block = Block::try_from(block).unwrap();
    let frame = block.read_frame_data().unwrap();
    let is_keyframe = frame.iter().any(|frame| is_key_frame(&frame.data));

    is_keyframe
}

pub fn block_group_relative_time(block_group: &MatroskaSpec) -> Result<i16, Box<dyn std::error::Error>> {
    let MatroskaSpec::BlockGroup(Master::Full(block_group)) = block_group else {
        return Err("not a block group".into());
    };

    let block = block_group.into_iter().find_map(|tag| {
        if let MatroskaSpec::Block(block) = tag {
            Some(block)
        } else {
            None
        }
    });

    let Some(block) = block else {
        return Err("no block found in block group".into());
    };

    let block = Block::try_from(block).unwrap();

    Ok(block.timestamp)
}

#[derive(Clone, Debug)]
pub struct BlockGroup<'a> {
    /// Raw frame data used to create the block (avoids the extra allocation of using owned_frame_data)
    frame_data: &'a [u8],

    pub track: u64,
    pub timestamp: i16,

    pub keyframe: bool, // Add this field

    pub invisible: bool,
    pub lacing: Option<BlockLacing>,
}

impl<'a> BlockGroup<'a> {
    pub fn new(track: u64, timestamp: i16, frame_data: &'a [u8], keyframe: bool) -> Self {
        Self {
            track,
            timestamp,
            invisible: false,
            keyframe,
            lacing: None,
            frame_data,
        }
    }
}

impl<'a> From<BlockGroup<'a>> for MatroskaSpec {
    fn from(block: BlockGroup<'a>) -> Self {
        let mut flags: u8 = 0x00;
        if block.invisible {
            flags |= 0x08;
        }

        if block.lacing.is_some() {
            match block.lacing.unwrap() {
                BlockLacing::Xiph => {
                    flags |= 0x02;
                }
                BlockLacing::Ebml => {
                    flags |= 0x06;
                }
                BlockLacing::FixedSize => {
                    flags |= 0x04;
                }
            }
        }

        if block.keyframe {
            flags |= 0x10;
        }

        let data = block.frame_data;
        let mut result = Vec::with_capacity(data.len() + 11);
        result.extend_from_slice(&block.track.as_vint().expect("Unable to convert track value to vint"));
        result.extend_from_slice(&block.timestamp.to_be_bytes());
        result.extend_from_slice(&flags.to_be_bytes());
        result.extend_from_slice(data);

        MatroskaSpec::Block(result)
    }
}

pub struct BlockGroupTagTimeInformation<'a> {
    pub relative_time: i16,
    pub absolute_time: u64,
    pub is_keyframe: bool,
    pub block_group: &'a [MatroskaSpec],
}
