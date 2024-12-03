use std::num::TryFromIntError;

use cadeau::xmf::vpx::is_key_frame;
use ebml_iterable::specs::Master;
use webm_iterable::matroska_spec::{Block, MatroskaSpec};

pub(crate) struct BlockGroup<'a> {
    cluster_timestamp: u64,
    block_group: &'a MatroskaSpec,
}

impl<'a> BlockGroup<'a> {
    pub(crate) fn new(block_group: &'a MatroskaSpec, cluster_timestamp: u64) -> Self {
        let MatroskaSpec::BlockGroup(Master::Full(_)) = block_group else {
            panic!("BlockGroup expected, got {:?}", block_group);
        };

        Self {
            block_group,
            cluster_timestamp,
        }
    }

    pub(crate) fn absolute_timestamp(&self) -> Result<u64, TryFromIntError> {
        let timestamp = u64::try_from(self.timestamp())?;
        Ok(self.cluster_timestamp + timestamp)
    }

    pub(crate) fn timestamp(&self) -> i16 {
        self.get_children()
            .iter()
            .find_map(|tag| {
                if let MatroskaSpec::Block(block) = tag {
                    let block = Block::try_from(block).unwrap();
                    Some(block.timestamp)
                } else {
                    None
                }
            })
            .unwrap()
    }

    pub(crate) fn is_key_frame(&self) -> bool {
        self.get_children()
            .iter()
            .find_map(|tag| {
                if let MatroskaSpec::Block(block) = tag {
                    let block = Block::try_from(block).unwrap();
                    let frames = block.read_frame_data().expect("Failed to read frame data");
                    Some(frames.iter().any(|frame| is_key_frame(frame.data)))
                } else {
                    None
                }
            })
            .unwrap()
    }

    // we assume only one frame per block group for now
    pub(crate) fn get_frame(&self) -> Vec<u8> {
        self.get_children()
            .iter()
            .find_map(|tag| {
                if let MatroskaSpec::Block(block) = tag {
                    let block = Block::try_from(block).unwrap();
                    let frames = block.read_frame_data().expect("Failed to read frame data");
                    Some(frames[0].data.to_vec())
                } else {
                    None
                }
            })
            .unwrap()
    }

    fn get_children(&self) -> &[MatroskaSpec] {
        match self.block_group {
            MatroskaSpec::BlockGroup(Master::Full(children)) => children,
            _ => panic!("BlockGroup expected, got {:?}", self.block_group),
        }
    }
}
