#![allow(clippy::print_stdout)]
#![allow(clippy::unwrap_used)]

use std::env;
use std::fs::File;
use std::path::Path;
use std::process::exit;

use debug::matroska_spec_name;
use webm_iterable::matroska_spec::{Master, MatroskaSpec};
use webm_iterable::WriteOptions;

pub mod block_group;
pub mod debug;
pub mod webm_cutter;

fn san() {
    let dangling = {
        let a: u32 = 0;
        &a as *const u32
    };

    let value = unsafe { dangling.read() };

    println!("{value}");
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();
    let args: Vec<&str> = args.iter().skip(1).map(String::as_str).collect();
    let args = parse_arg(&args)?;

    // uncomment to sanity check if memory sanitizer works
    // san();

    // SAFETY: Just pray at this point.
    #[cfg(feature = "dlopen")]
    unsafe {
        cadeau::xmf::init(args.lib_xmf_path)?;
    }

    let path = Path::new(args.input_path);
    let mut cutter = webm_cutter::WebmCutter::new(path, args.cut_start)?;
    let mut writer = webm_iterable::WebmWriter::new(File::create(args.output_path)?);
    let writer_mut_ref = &mut writer;
    cutter.on_element(move |tag| {
        if let MatroskaSpec::Segment(Master::Start) = tag {
            writer_mut_ref.write_advanced(&tag, WriteOptions::is_unknown_sized_element())
        } else {
            writer_mut_ref.write(&tag)
        }
        .map_err(|e| {
            let tag_name = matroska_spec_name(&tag);
            println!("Error: {:?}", e);
            println!("Failed to write tag: {:?}", tag_name);
            format!("Failed to write tag: {:?}", tag_name)
        })?;

        Ok(())
    })?;

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

const HELP: &str = "Usage: cut -i <input> -o <output> --lib-xmf <libxmf.so> -c <start_time>";

fn parse_arg<'a>(mut value: &[&'a str]) -> Result<Args<'a>, &'static str> {
    let mut arg = Args::default();
    loop {
        match value {
            #[cfg(feature = "dlopen")]
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
                exit(0);
            }
            [] => break,
            _ => return Err("Invalid arguments"),
        }
    }

    Ok(arg)
}
