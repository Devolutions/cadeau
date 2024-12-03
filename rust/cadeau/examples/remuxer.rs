#![allow(unused_crate_dependencies)]
#![allow(clippy::print_stdout)]
#![allow(clippy::unwrap_used)]

use std::path::{Path, PathBuf};
use std::{env, io};

const USAGE: &str = "--in <PATH> [--out <PATH>] [--lib-xmf <PATH>]";

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

    let input_path = args
        .input_path
        .ok_or_else(|| io::Error::other("--in argument is required"))?;

    let default_output_path = {
        let mut path = PathBuf::from("./");

        if let Some(file_stem) = input_path.file_stem() {
            let mut file_name = file_stem.to_owned();
            file_name.push("-remuxed");
            path.push(file_name);
        } else {
            path.push("remuxed");
        }

        if let Some(extension) = input_path.extension() {
            path.set_extension(extension);
        }

        path
    };

    let output_path = args.output_path.unwrap_or(&default_output_path);

    // SAFETY: No initialisation or termination routine in the XMF library we should worry about for preconditions.
    #[cfg(feature = "dlopen")]
    unsafe {
        cadeau::xmf::init(args.lib_xmf_path)?;
    }

    assert!(cadeau::xmf::is_init());

    println!("Remuxing {} into {}...", input_path.display(), output_path.display());

    cadeau::xmf::muxer::webm_remux(input_path, output_path)?;

    println!("OK.");

    Ok(())
}

#[derive(Debug)]
struct Args<'a> {
    input_path: Option<&'a Path>,
    output_path: Option<&'a Path>,
    #[cfg_attr(not(feature = "dlopen"), allow(dead_code))]
    lib_xmf_path: &'a str,
    show_usage: bool,
}

fn parse_args<'a>(mut input: &[&'a str]) -> io::Result<Args<'a>> {
    let mut args = Args {
        lib_xmf_path: "./libxmf.so",
        input_path: None,
        output_path: None,
        show_usage: false,
    };

    loop {
        match input {
            ["--lib-xmf", value, rest @ ..] => {
                args.lib_xmf_path = value;
                input = rest;
            }
            ["--input" | "--in" | "-i", value, rest @ ..] => {
                args.input_path = Some(Path::new(*value));
                input = rest;
            }
            ["--output" | "--out" | "-o", value, rest @ ..] => {
                args.output_path = Some(Path::new(*value));
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
