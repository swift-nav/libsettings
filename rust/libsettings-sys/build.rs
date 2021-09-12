fn main() {
    let dst = cmake::Config::new("../..")
        .define("SKIP_UNIT_TESTS", "ON")
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("libsettings_ENABLE_PYTHON", "OFF")
        .build();
    println!("cargo:rustc-link-search=native={}/lib", dst.display());

    let dst = cmake::Config::new(".").build();
    println!("cargo:rustc-link-search=native={}/lib", dst.display());

    println!("cargo:rustc-link-lib=static=sbp");
    println!("cargo:rustc-link-lib=static=settings");
    println!("cargo:rustc-link-lib=static=swiftnav");
    println!("cargo:rustc-link-lib=static=rustbindsettings");

    let bindings = bindgen::Builder::default()
        .header("./libsettings_wrapper.h")
        .blocklist_item("FP__.*")
        .clang_arg("-I../../include")
        .clang_arg("-I../../third_party/libswiftnav/include")
        .clang_arg("-I../../third_party/libsbp/c/include")
        .generate()
        .expect("Unable to generate bindings");

    // Write out the generated bindings...
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let out_dir = std::path::PathBuf::from(out_dir);

    bindings
        .write_to_file(out_dir.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
