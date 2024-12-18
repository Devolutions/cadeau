#![allow(unused_crate_dependencies)]
#![allow(clippy::print_stdout)]
#![allow(clippy::unwrap_used)]

use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;
use std::time::{SystemTime, UNIX_EPOCH};
use std::{env, io};

use cadeau::xmf::image::Image;
use cadeau::xmf::recorder::Recorder;

const USAGE: &str = "[--lib-xmf <PATH>] [--capture-folder <PATH>]";

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();
    let args: Vec<&str> = args.iter().skip(1).map(String::as_str).collect();
    let args = parse_args(&args)?;

    if args.show_usage {
        let prgm_name = env::args().next().unwrap();
        println!("Usage: {prgm_name} {USAGE}");
        return Ok(());
    } else {
        println!("{args:?}");
    }

    // SAFETY: No initialisation or termination routine in the XMF library we should worry about for preconditions.
    #[cfg(feature = "dlopen")]
    unsafe {
        cadeau::xmf::init(args.lib_xmf_path)?;
    }

    assert!(cadeau::xmf::is_init());

    let psv_path = args.capture_path.join("frame_meta.psv");
    let (_, records) = parse_psv_file(&psv_path)?;

    let output_video_path = args.capture_path.join("video.webm");

    let base_time = get_tick_count64();

    let mut recorder = Recorder::builder(1920, 1080)
        .frame_rate(10)
        .current_time(base_time)
        .init(output_video_path)?;

    for record in records {
        let frame_time: u64 = record[0].parse()?;
        let current_time = base_time + frame_time;

        let image_file = args.capture_path.join(&record[2]);
        let image = Image::load_file(&image_file)?;

        println!("image: {}x{}, time: {}", image.width(), image.height(), frame_time);

        recorder.set_current_time(current_time);
        recorder.update_frame(image.data(), 0, 0, image.width(), image.height(), image.step())?;
        recorder.timeout();
    }

    Ok(())
}

fn get_tick_count64() -> u64 {
    let now = SystemTime::now();
    let boot_time = now.duration_since(UNIX_EPOCH).unwrap();

    boot_time.as_secs() * 1000 + u64::from(boot_time.subsec_millis())
}

#[derive(Debug)]
struct Args<'a> {
    lib_xmf_path: &'a str,
    capture_path: &'a Path,
    show_usage: bool,
}

impl Default for Args<'_> {
    fn default() -> Self {
        Self {
            lib_xmf_path: "./libxmf.so",
            capture_path: Path::new("./capture_sample"),
            show_usage: false,
        }
    }
}

fn parse_args<'a>(mut input: &[&'a str]) -> io::Result<Args<'a>> {
    let mut args = Args::default();

    loop {
        match input {
            ["--lib-xmf", value, rest @ ..] => {
                args.lib_xmf_path = value;
                input = rest;
            }
            ["--capture-folder", value, rest @ ..] => {
                args.capture_path = Path::new(*value);
                input = rest;
            }
            ["--help" | "-h", rest @ ..] => {
                args.show_usage = true;
                input = rest;
            }
            [unexpected_arg, ..] => {
                return Err(io::Error::new(
                    io::ErrorKind::Other,
                    format!("unexpected argument: {unexpected_arg}"),
                ))
            }
            [] => break,
        }
    }

    Ok(args)
}

fn parse_psv_file(path: &Path) -> io::Result<(Vec<String>, Vec<Vec<String>>)> {
    let file = File::open(path).map(BufReader::new)?;

    let mut lines = file.lines();

    let headers = lines.next().ok_or_else(|| io::Error::other("empty PSV file"))??;
    let headers: Vec<String> = headers.split('|').map(|o| o.to_owned()).collect();

    let records = lines
        .map_while(Result::ok)
        .map(|line| line.split('|').map(|o| o.to_owned()).collect())
        .collect();

    Ok((headers, records))
}
