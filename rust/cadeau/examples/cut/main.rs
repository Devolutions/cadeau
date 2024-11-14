#![allow(clippy::print_stdout)]
#![allow(clippy::unwrap_used)]

use std::{env, fs::File, io, thread, time::Duration};

use debug::mastroka_spec_name;
use webm_iterable::{
    matroska_spec::{Master, MatroskaSpec},
    WriteOptions,
};

pub mod block_group;
pub mod debug;
pub mod webm_cutter;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Args = env::args()
        .collect::<Vec<String>>()
        .try_into()
        .map_err(|e| format!("error: {}", e))?;

    // SAFETY: Just pray at this point.
    #[cfg(feature = "dlopen")]
    unsafe {
        cadeau::xmf::init(
            args.lib_xmf_path
                .as_ref()
                .expect("xmf dll path is needed for dynamic loading"),
        )?;
    }

    let mut cutter = webm_cutter::WebmCutter::new(&args.input_path, args.cut_start)?;
    let mut writer = webm_iterable::WebmWriter::new(File::create(&args.output_path)?);

    let (rx, tx) = std::sync::mpsc::channel();
    cutter.on_element(|tag| {
        rx.send(tag).unwrap();
    })?;

    let mut writter = thread::spawn(move || {
        while let Ok(tag) = tx.recv_timeout(Duration::from_secs(2)) {
            if let MatroskaSpec::Segment(Master::Start) = tag {
                if let Err(e) = writter.write_advanced(&tag, WriteOptions::is_unknown_sized_element()) {
                    let tag_name = mastroka_spec_name(&tag);
                    println!("error: failed to write tag: {}", tag_name);
                    println!("error: {:?}", e);
                }
                continue;
            }

            if let Err(e) = writter.write(&tag) {
                let tag_name = mastroka_spec_name(&tag);
                println!("error: failed to write tag: {}", tag_name);
                println!("error: {}", e);
                break;
            }
        }
        writter
    })
    .join()
    .map_err(|_| io::Error::new(io::ErrorKind::Other, "thread join error"))?;

    println!("done,flushing");
    writter.flush().unwrap();
    Ok(())
}

struct Args {
    // input path, -i
    input_path: String,
    // lib_xmf path, --lib-xmf
    #[cfg(feature = "dlopen")]
    lib_xmf_path: Option<String>,
    // output path, -o
    output_path: String,
    // -c or --cut,cut start time in seconds
    cut_start: u32,
}

impl TryFrom<Vec<String>> for Args {
    type Error = &'static str;

    fn try_from(value: Vec<String>) -> Result<Self, Self::Error> {
        if value.iter().any(|v| v == "-h" || v == "--help") {
            println!("Usage: cut -i <input> -o <output> --lib-xmf <libxmf.so> --cut-start <start_time>");
            std::process::exit(0);
        }

        // search for -i
        let input_path = value
            .iter()
            .enumerate()
            .find(|(_, v)| *v == "-i")
            .filter(|(i, _)| i + 1 < value.len())
            .map(|(i, _)| value[i + 1].clone())
            .ok_or("missing input path")?;

        #[cfg(feature = "dlopen")]
        let lib_xmf_path = value
            .iter()
            .enumerate()
            .find(|(_, v)| *v == "--lib-xmf")
            .filter(|(i, _)| i + 1 < value.len())
            .map(|(i, _)| value[i + 1].clone());

        let output_path = value
            .iter()
            .enumerate()
            .find(|(_, v)| *v == "-o")
            .filter(|(i, _)| i + 1 < value.len())
            .map(|(i, _)| value[i + 1].clone())
            .ok_or("missing output path")?;

        let cut_start = value
            .iter()
            .enumerate()
            .find(|(_, v)| *v == "--cut" || *v == "-c")
            .filter(|(i, _)| i + 1 < value.len())
            .map(|(i, _)| value[i + 1].parse::<u32>())
            .ok_or("missing cut start time")?
            .map_err(|_| "cut start time must be a number")?;

        Ok(Self {
            input_path,
            #[cfg(feature = "dlopen")]
            lib_xmf_path,
            output_path,
            cut_start,
        })
    }
}
