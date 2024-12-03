#![allow(clippy::print_stdout)]
#![allow(clippy::unwrap_used)]

mod block_group;
mod debug;
mod webm_cutter;

use std::fs::File;
use std::path::Path;
use std::{env, io};

use webm_iterable::matroska_spec::{Master, MatroskaSpec};
use webm_iterable::WriteOptions;

use self::debug::matroska_spec_name;

const USAGE: &str = "--in <PATH> --out <PATH> --lib-xmf <PATH> --cut <CUT START>";

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();
    let args: Vec<&str> = args.iter().skip(1).map(String::as_str).collect();
    let args = parse_arg(&args)?;

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

    let mut cutter = webm_cutter::WebmCutter::new(Path::new(args.input_path), args.cut_start)?;
    let mut writer = webm_iterable::WebmWriter::new(File::create(args.output_path)?);

    println!("Cutting...");

    cutter.on_element({
        let writer = &mut writer;

        move |tag| {
            if let MatroskaSpec::Segment(Master::Start) = tag {
                writer.write_advanced(&tag, WriteOptions::is_unknown_sized_element())
            } else {
                writer.write(&tag)
            }
            .map_err(|e| {
                let tag_name = matroska_spec_name(&tag);
                println!("Error: {:?}", e);
                println!("Failed to write tag: {:?}", tag_name);
                format!("Failed to write tag: {:?}", tag_name)
            })?;

            Ok(())
        }
    })?;

    println!("Flushing...");

    writer.flush()?;

    println!("OK.");

    Ok(())
}

#[derive(Debug, Default)]
struct Args<'a> {
    input_path: &'a str,
    #[cfg_attr(not(feature = "dlopen"), allow(dead_code))]
    lib_xmf_path: &'a str,
    output_path: &'a str,
    cut_start: u32,
    show_usage: bool,
}

fn parse_arg<'a>(mut input: &[&'a str]) -> io::Result<Args<'a>> {
    let mut args = Args::default();

    loop {
        match input {
            ["--lib-xmf", lib_xmf_path, rest @ ..] => {
                args.lib_xmf_path = lib_xmf_path;
                input = rest;
            }
            ["--input" | "--in" | "-i", input_path, rest @ ..] => {
                args.input_path = input_path;
                input = rest;
            }
            ["--output" | "--out" | "-o", output_path, rest @ ..] => {
                args.output_path = output_path;
                input = rest;
            }
            ["--cut" | "-c", cut_start, rest @ ..] => {
                args.cut_start = cut_start
                    .parse::<u32>()
                    .map_err(|_| io::Error::other("cut start time must be a number"))?;
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
