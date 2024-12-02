fn main() {
    // Link with the C++ standard library
    println!("cargo:rustc-link-lib=stdc++");
    // Link with libvpx
    println!("cargo:rustc-link-lib=vpx");
    // Specify the path to the libvpx library
    println!("cargo:rustc-link-search=native=/home/irving/.conan/data/libvpx/1.10.0/devolutions/stable/package/011fa63773d8245fd1d8aa198a0f8c3d677549b9/lib");
}
