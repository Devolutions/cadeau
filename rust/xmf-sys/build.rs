use std::env;

#[derive(Debug)]
enum Strategy {
    StaticLinking,
    DynamicLinking,
    DynamicLoading,
}

fn resolve_strategy() -> Strategy {
    // "dlopen" feature have the highest precedence.
    // Indeed, we don’t have the `extern "C"` block when this feature is set.
    if env::var("CARGO_FEATURE_DLOPEN").is_ok() {
        return Strategy::DynamicLoading;
    }

    // Check for env variables first.
    if env::var("XMF_STATIC").is_ok() {
        return Strategy::StaticLinking;
    }
    if env::var("XMF_DYNAMIC").is_ok() {
        return Strategy::DynamicLinking;
    }

    // Check for the "static" feature last.
    if env::var("CARGO_FEATURE_STATIC").is_ok() {
        return Strategy::StaticLinking;
    }

    // Default to dynamic linking otherwise.
    Strategy::DynamicLinking
}

fn main() {
    println!("cargo::rerun-if-env-changed=XMF_STATIC");
    println!("cargo::rerun-if-env-changed=XMF_DYNAMIC");
    println!("cargo::rerun-if-env-changed=XMF_SEARCH_PATH");

    let strategy = resolve_strategy();

    match strategy {
        Strategy::StaticLinking => {
            println!("cargo:rustc-link-lib=static=xmf");
        }
        Strategy::DynamicLinking => {
            println!("cargo:rustc-link-lib=dylib=xmf");
        }
        Strategy::DynamicLoading => {
            // Do not link to anything.
            // Native library loading must be handled programmatically in the final binary.
        }
    }

    if let Ok(search_path) = env::var("XMF_SEARCH_PATH") {
        println!("cargo::rustc-link-search={search_path}");
    }

    #[cfg(feature = "verbose")]
    {
        build_print::println!("xmf-sys: build with strategy: {:?}", strategy);
        build_print::println!("xmf-sys: search path: {:?}", env::var("XMF_SEARCH_PATH"));
    }
}
