use std::{io::Seek, path::Path};

use cadeau::xmf::vpx::{
    decoder::{VpxDecoder, VpxDecoderConfig},
    encoder::{VpxEncoder, VpxEncoderConfig},
    is_key_frame, VpxCodec,
};
use ebml_iterable::tools::Vint;
use webm_iterable::{
    matroska_spec::{Block, BlockLacing, Master, MatroskaSpec, SimpleBlock},
    WebmIterator,
};
use xmf_sys::vpx::VPX_EFLAG_FORCE_KF;

use crate::block_group::BlockGroup;

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

        let width = u32::try_from(width)?;
        let height = u32::try_from(height)?;

        let decoder_config = VpxDecoderConfig::builder()
            .codec(codec)
            .width(width)
            .height(height)
            .threads(3)
            .build();

        let encoder_config = VpxEncoderConfig::builder()
            .codec(codec)
            .threads(3)
            .width(width)
            .height(height)
            .timebase_num(1)
            .timebase_den(1000) // 1ms
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

    pub fn on_element(&mut self, write: impl Fn(MatroskaSpec)) -> Result<(), Box<dyn std::error::Error>> {
        while !self.headers.is_empty() {
            let tag = self.headers.remove(0);
            write(tag);
        }
        println!("writing headers done");

        // Based on Webm Muxer Guide line, the first block in a cluster must be a keyframe.
        let cluster_offset = self.find_cluster_at_cut_time()?;

        self.seek_iterator_to(cluster_offset, &[MatroskaSpec::BlockGroup(Master::Start)])?;
        let cluster_start = self.next_tag().expect("no next tag found")?;
        assert!(matches!(cluster_start, MatroskaSpec::Cluster(Master::Start)));
        let timestamp = self.next_tag().expect("no next tag found")?;
        let timestamp = match timestamp {
            MatroskaSpec::Timestamp(ts) => ts,
            _ => return Err("no timestamp found".into()),
        };
        let mut block_groups = Vec::new();
        while let Some(Ok(tag)) = self.next_tag() {
            match tag {
                MatroskaSpec::Cluster(_) | MatroskaSpec::Timestamp(_) => {
                    break;
                }
                MatroskaSpec::BlockGroup(Master::Full(_)) => {
                    block_groups.push(tag);
                }
                _ => {}
            }
        }

        let mut cluster_start_written = false;
        let mut time_offset = 0;
        for window in block_groups.windows(2) {
            let [current, next] = window else {
                panic!("window size is not 2");
            };

            let current_block_group = BlockGroup::new(current, timestamp);
            let next_block_group = BlockGroup::new(next, timestamp);

            let pts = current_block_group.absolute_timestamp()?;
            let duration = next_block_group.absolute_timestamp()? - pts;

            // we assume only one frame per block group for now and there's no complicated lacing and frame dependencies
            self.decoder.decode(current_block_group.get_frame().as_ref())?;
            let image = self.decoder.next_frame()?;
            self.encoder.encode_frame(
                &image,
                pts.try_into().unwrap(),
                usize::try_from(duration)?,
                VPX_EFLAG_FORCE_KF,
            )?; // we make every frame a keyframe inside this cluster
            let frame = self.encoder.next_frame()?.ok_or("no frame found")?;
            let is_key_frame = is_key_frame(&frame);

            println!(
                "encoding blockgroup: 
                pts: {},
                duration: {},
                absolute_timestamp: {},
                original frame is key frame: {},
                encoded frame is key frame: {}",
                pts,
                duration,
                current_block_group.absolute_timestamp()?,
                current_block_group.is_key_frame(),
                is_key_frame
            );

            if current_block_group.absolute_timestamp()? < self.cut_time_in_ms() {
                continue;
            }

            if !cluster_start_written {
                let cluster = MatroskaSpec::Cluster(Master::Start);
                write(cluster);
                let timestamp = MatroskaSpec::Timestamp(0);
                time_offset = current_block_group.absolute_timestamp()?;
                write(timestamp);
                cluster_start_written = true;
            }

            let block = SimpleBlock::new_uncheked(
                &frame,
                1,
                current_block_group.timestamp() - i16::try_from(time_offset)?,
                false,
                None,
                false,
                true,
            );
            write(block.into());
        }

        let cluster_end = MatroskaSpec::Cluster(Master::End);
        write(cluster_end);
        while let Some(Ok(tag)) = self.next_tag() {
            if let MatroskaSpec::Timestamp(time) = tag {
                let tag = MatroskaSpec::Timestamp(time - time_offset);
                write(tag);
            } else {
                write(tag);
            }
        }

        Ok(())
    }

    fn cut_time_in_ms(&self) -> u64 {
        let cutime_in_nanoseconds = u64::from(self.cut_time) * 1_000_000_000;
        cutime_in_nanoseconds / self.time_stamp_scale
    }

    fn seek_iterator_to(
        &mut self,
        offset: usize,
        tags_to_buffer: &[MatroskaSpec],
    ) -> Result<(), Box<dyn std::error::Error>> {
        let iterator = self.iterator.take().unwrap();
        let mut file = iterator.into_inner();
        file.seek(std::io::SeekFrom::Start(offset as u64))?;
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

    fn find_cluster_at_cut_time(&mut self) -> Result<usize, Box<dyn std::error::Error>> {
        self.seek_iterator_to(0, &[MatroskaSpec::BlockGroup(Master::Start)])?;
        let mut cluster_offset = 0;
        let mut cluster_cut_time = 0;

        while let Some(Ok(tag)) = self.next_tag() {
            if let MatroskaSpec::Cluster(Master::Start) = &tag {
                cluster_offset = self.tag_offset();
            };

            if let MatroskaSpec::Timestamp(time) = tag {
                cluster_cut_time = time;
            }

            let MatroskaSpec::BlockGroup(Master::Full(_)) = tag else {
                continue;
            };

            let block_group = BlockGroup::new(&tag, cluster_cut_time);

            if block_group.absolute_timestamp()? > self.cut_time_in_ms() {
                return Ok(cluster_offset);
            }
        }

        Err("no cluster found".into())
    }
}

pub fn is_key_frame_block_group(block_group: &MatroskaSpec) -> bool {
    let MatroskaSpec::BlockGroup(Master::Full(block_group)) = block_group else {
        return false;
    };

    let block = block_group.iter().find_map(|tag| {
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
    let is_keyframe = frame.iter().any(|frame| is_key_frame(frame.data));

    is_keyframe
}

pub fn block_group_relative_time(block_group: &MatroskaSpec) -> Result<i16, Box<dyn std::error::Error>> {
    let MatroskaSpec::BlockGroup(Master::Full(block_group)) = block_group else {
        return Err("not a block group".into());
    };

    let block = block_group.iter().find_map(|tag| {
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
pub struct WriteBlockGroup<'a> {
    /// Raw frame data used to create the block (avoids the extra allocation of using owned_frame_data)
    frame_data: &'a [u8],

    pub track: u64,
    pub timestamp: i16,

    pub keyframe: bool, // Add this field

    pub invisible: bool,
    pub lacing: Option<BlockLacing>,
}

impl<'a> WriteBlockGroup<'a> {
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

impl<'a> From<WriteBlockGroup<'a>> for MatroskaSpec {
    fn from(block: WriteBlockGroup<'a>) -> Self {
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
