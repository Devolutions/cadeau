#![doc = include_str!("../README.md")]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]

#[cfg(test)]
use {ebml_iterable as _, webm_iterable as _};

#[cfg(feature = "xmf")]
pub mod xmf;
