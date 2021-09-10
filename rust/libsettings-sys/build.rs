fn main() {
    cmake::Config::new("../..")
        .define("SKIP_UNIT_TESTS", "ON")
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("libsettings_ENABLE_PYTHON", "OFF")
        .build();

    let dst = cmake::Config::new(".").build();
    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:rustc-link-lib=static=sbp");
    println!("cargo:rustc-link-lib=static=settings");

    let bindings = bindgen::Builder::default()
        .header("./libsettings_wrapper.h")
        .clang_arg("-I../../include")
        .generate()
        .expect("Unable to generate bindings");

    // Write out the generated bindings...
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let out_dir = std::path::PathBuf::from(out_dir);

    bindings
        .write_to_file(out_dir.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
