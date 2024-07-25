# An idiomatic cadeau to Rust

Idiomatic wrapper around Cadeau library, performance primitives and media foundation functions.

It’s possible to choose between dynamically loading the library at runtime by enabling the `dlopen` feature, or regular static / dynamic linkage at build-time.
The API itself is identical except for a `init` function which must be called before using other API when `dlopen` feature is enabled.

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
