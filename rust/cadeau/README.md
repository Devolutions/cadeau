# An efficient and idiomatic cadeau to Rust

Idiomatic wrapper around the Cadeau library: performance primitives and media foundation.

It’s possible to choose between dynamically loading the library and regular static / dynamic linking at build-time for a total of three options.
See [xmf-sys](https://crates.io/crates/xmf-sys) to learn more about this.

## Read decoded VPX pixels

`VpxImage::i420_planes()` borrows the Y, U and V planes of an 8-bit I420 image without copying.
Use each plane's `rows()` iterator to read its visible pixels, and `stride()` to inspect the native row spacing.
The plane does not expose a single byte slice: libvpx may leave the padding between rows uninitialized.
The row slices keep the image borrowed, so the decoder cannot decode again while they are in use.

## Example: generate a WebM file from a PNG image

```rust,no_run
use cadeau::xmf::image::Image;
use cadeau::xmf::recorder::Recorder;

fn main() -> Result<(), Box<dyn std::error::Error>> {
  #[cfg(feature = "dlopen")]
  unsafe { cadeau::xmf::init("libxmf.so")? };

  assert!(cadeau::xmf::is_init());

  let frame = Image::load_file("frame.png")?;

  let mut recorder = Recorder::builder(frame.width(), frame.height())
    .frame_rate(10)
    .init("output.webm")?;

  recorder.update_frame(frame.data(), 0, 0, frame.width(), frame.height(), frame.step())?;
  recorder.timeout();

  Ok(())
}
```
