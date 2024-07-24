#![doc = include_str!("../README.md")]
#![allow(clippy::too_many_arguments)]
#![allow(non_snake_case)]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]

mod macros;

#[cfg(feature = "xmf")]
pub mod xmf;
