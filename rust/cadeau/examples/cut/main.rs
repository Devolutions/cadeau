#![allow(clippy::print_stdout)]
#![allow(clippy::unwrap_used)]

use std::{
    env,
    fs::File,
    io,
    sync::{Arc, Mutex},
    thread,
    time::Duration,
};

use debug::matroska_spec_name;
use webm_iterable::{
    matroska_spec::{Master, MatroskaSpec},
    WriteOptions,
};

pub mod block_group;
pub mod debug;
pub mod webm_cutter;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();
    let args: Vec<&str> = args.iter().skip(1).map(String::as_str).collect();
    let args = parse_arg(&args)?;

    // SAFETY: Just pray at this point.
    #[cfg(feature = "dlopen")]
    unsafe {
        cadeau::xmf::init(args.lib_xmf_path)?;
    }

    let mut cutter = webm_cutter::WebmCutter::new(&args.input_path, args.cut_start)?;
    let mut writer = webm_iterable::WebmWriter::new(File::create(&args.output_path)?);

    let (sender, receiver) = std::sync::mpsc::sync_channel(10);
    let receiver = Arc::new(Mutex::new(receiver));
    cutter.on_element(|tag| {
        sender.send(tag).unwrap();
    })?;

    thread::scope(|s| {
        s.spawn(|| {
            while let Ok(tag) = receiver.lock().unwrap().recv_timeout(Duration::from_secs(2)) {
                if let MatroskaSpec::Segment(Master::Start) = tag {
                    if let Err(e) = writer.write_advanced(&tag, WriteOptions::is_unknown_sized_element()) {
                        let tag_name = matroska_spec_name(&tag);
                        println!("error: failed to write tag: {}", tag_name);
                        println!("error: {:?}", e);
                    }
                    continue;
                }

                if let Err(e) = writer.write(&tag) {
                    let tag_name = matroska_spec_name(&tag);
                    println!("error: failed to write tag: {}", tag_name);
                    println!("error: {}", e);
                    break;
                }
            }
        });
    });

    println!("done,flushing");
    writer.flush().unwrap();
    Ok(())
}

#[derive(Debug, Default)]
struct Args<'a> {
    // input path, -i
    input_path: &'a str,
    // lib_xmf path, --lib-xmf
    #[cfg(feature = "dlopen")]
    lib_xmf_path: &'a str,
    // output path, -o
    output_path: &'a str,
    // -c or --cut,cut start time in seconds
    cut_start: u32,
}

const HELP: &'static str = "Usage: cut -i <input> -o <output> --lib-xmf <libxmf.so> --cut-start <start_time>";

fn parse_arg<'a>(mut value: &[&'a str]) -> Result<Args<'a>, &'static str> {
    let mut arg = Args::default();
    loop {
        match value {
            ["--lib-xmf", lib_xmf_path, rest @ ..] => {
                arg.lib_xmf_path = lib_xmf_path;
                value = rest;
            }
            ["--input" | "-i", input_path, rest @ ..] => {
                arg.input_path = input_path;
                value = rest;
            }
            ["--output" | "-o", output_path, rest @ ..] => {
                arg.output_path = output_path;
                value = rest;
            }
            ["--cut" | "-c", cut_start, rest @ ..] => {
                arg.cut_start = cut_start
                    .parse::<u32>()
                    .map_err(|_| "cut start time must be a number")?;
                value = rest;
            }
            ["--help" | "-h", ..] => {
                println!("{}", HELP);
                std::process::exit(0);
            }
            [] => break,
            _ => return Err("Invalid arguments"),
        }
    }

    Ok(arg)
}
