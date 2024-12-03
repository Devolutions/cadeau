use std::io::Seek;
use std::path::Path;

use cadeau::xmf::vpx::{is_key_frame, VpxCodec, VpxDecoder, VpxEncoder};
use webm_iterable::matroska_spec::{Master, MatroskaSpec, SimpleBlock};
use webm_iterable::WebmIterator;
use xmf_sys::VPX_EFLAG_FORCE_KF;

use crate::block_group::BlockGroup;
use crate::debug::matroska_spec_name;

pub(crate) struct WebmCutter {
    encoder: VpxEncoder,
    decoder: VpxDecoder,
    iterator: Option<WebmIterator<std::fs::File>>,
    cut_time: u32,
    headers: Vec<MatroskaSpec>,
    time_stamp_scale: u64,
}

// Cutter for webm videos with only video tracks and no cues.
impl WebmCutter {
    pub(crate) fn new(input: &Path, cut_time: u32) -> Result<Self, Box<dyn std::error::Error>> {
        let file = std::fs::File::open(input)?;
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

        let decoder = VpxDecoder::builder()
            .codec(codec)
            .width(width)
            .height(height)
            .threads(3)
            .build()?;

        let encoder = VpxEncoder::builder()
            .codec(codec)
            .threads(3)
            .width(width)
            .height(height)
            .timebase_num(1)
            .timebase_den(1000) // 1ms
            .bitrate(256 * 1024) // Set to 256Kbps
            .build()?;

        Ok(Self {
            encoder,
            decoder,
            iterator: Some(webm_iterator),
            cut_time,
            headers,
            time_stamp_scale,
        })
    }

    pub(crate) fn on_element(
        &mut self,
        mut write: impl FnMut(MatroskaSpec) -> Result<(), Box<dyn std::error::Error>>,
    ) -> Result<(), Box<dyn std::error::Error>> {
        while !self.headers.is_empty() {
            let tag = self.headers.remove(0);
            let tag_name = matroska_spec_name(&tag);
            println!("Writing header: {tag_name}...");
            write(tag)?;
        }

        println!("Done writing headers.");

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
            {
                let image = self.decoder.next_frame()?;
                self.encoder.encode_frame(
                    &image,
                    pts.try_into().unwrap(),
                    usize::try_from(duration)?,
                    VPX_EFLAG_FORCE_KF,
                )?; // we make every frame a keyframe inside this cluster
            }
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
                write(cluster)?;
                let timestamp = MatroskaSpec::Timestamp(0);
                time_offset = current_block_group.absolute_timestamp()?;
                write(timestamp)?;
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
            write(block.into())?;
        }

        let cluster_end = MatroskaSpec::Cluster(Master::End);
        write(cluster_end)?;
        while let Some(Ok(tag)) = self.next_tag() {
            if let MatroskaSpec::Timestamp(time) = tag {
                let tag = MatroskaSpec::Timestamp(time - time_offset);
                write(tag)?;
            } else {
                write(tag)?;
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
