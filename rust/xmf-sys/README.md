# Rust bindings to XMF: eXtreme Media Foundation, an efficiently dangerous cadeau to Rust

Low-level and unidiomatic bindings to the XMF library.

It’s possible to choose between dynamically loading the library and regular static / dynamic linking at build-time for a total of three options.

## Dynamic Linking

- Feature flag: none
- Env variable: `XMF_DYNAMIC`

The shared library is not required when building the binary, but is required when executing it.

## Static Linking

- Feature flag: `static`
- Env variable: `XMF_STATIC`

The static library is required when building the binary, and no additional dependency is required when executing the binary.
You can specify a search path for the static library using `XMF_SEARCH_PATH` env variable.

## Dynamic Loading

- Feature flag: `dlopen`
- Env variable: none

The shared library is not required when building the binary, and is not required when executing the binary either.
However, you need to programmatically load the file before using any FFI function by using the `init` function.
The rest of the API is identical except for this `init` function.
You can check dynamically if the library has been loaded using the `is_init` function.
This function is annotated with `#[inline]` and compiles down to `true` when the `dlopen` feature is not enabled.

## Native Library Binaries

Available at the [release page of the GitHub repository](https://github.com/Devolutions/cadeau/releases).
