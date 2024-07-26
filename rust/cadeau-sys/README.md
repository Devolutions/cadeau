# A dangerously unsafe cadeau to Rust

:warn: This crate has been replaced by [xmf-sys](https://crates.io/crates/xmf-sys) :warn:

Low-level and unidiomatic bindings to Cadeau library, performance primitives and media foundation functions.

It’s possible to choose between dynamically loading the library at runtime by enabling the `dlopen` feature, or regular static / dynamic linkage at build-time.
The API itself is identical except for a `init` function which must be called before using other API when `dlopen` feature is enabled.
