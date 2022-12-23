# Cadeau: performance primitives and media foundation

Cadeau 🎁 (pronounced /kado/, the French word for "gift") is a project providing reusable performance primitives and media foundation functions. Dealing with pixel buffers is tricky (color conversion, copying/moving regions, comparing framebuffers, etc), but it also tends to complexify the build system with special tooling. Loading and saving common image file formats, or compressing framebuffers into a playable video file format, requires third-party libraries that make dependency management harder. What if you could solve those issues *once* and remove the burden from consumer projects? That's what Cadeau is - a true gift!

## XPP: eXtreme Performance Primitives

Core performance primitivates inspired by the [Intel Integrated Performance Primitives](https://www.intel.com/content/www/us/en/developer/tools/oneapi/ipp.html). This library is designed to provide optimized image processing functions with minimal dependencies (libc only), using three approaches:

 * generic: naive, unoptimized implementation in pure C
 * SIMD: optimized implementation using SIMD intrinsics in C
 * Halide: optimized implementation using SIMD and Halide AOT

[Halide](https://halide-lang.org/) is a fairly complex *build-time* dependency, but we use it to generate functions ahead-of-time (AOT) rather than just-in-time (JIT). The resulting code is linked within the library with a minimal Halide runtime. The CMake build system streamlines Halide usage so you don't have to deal with it directly, and benefit from optimized functions that frequently *beat* their handcrafted SIMD equivalents. Yes, Halide is THAT good!

## XMF: eXtreme Media Foundation

Media foundation library built on top of the core performance primitives that supports image and video formats, using third-party dependencies:

 * JPEG: libjpeg-turbo
 * PNG: libpng + zlib
 * WebM: libvpx + libwebm

libxmf is built as a shared library to faciliate importing all dependencies at once, but also to make it possible to load it as an optional runtime component. Advanced users may prefer to link everything statically into their application and that's fine too, it's just a bit more work.
