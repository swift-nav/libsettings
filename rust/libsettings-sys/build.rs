use cmake::Config;

fn main() {
    // Builds the project in the root directory, installing it
    // into $OUT_DIR
    let mut dst = Config::new("../..")
        .define("SKIP_UNIT_TESTS", "ON")
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("libsettings_ENABLE_PYTHON", "OFF")
        .build();
    println!("cargo:rustc-link-search=native={}/lib", dst.display());

    dst = Config::new(".").build();
    println!("cargo:rustc-link-search=native={}/lib", dst.display());

    println!("cargo:rustc-link-lib=static=sbp");
    println!("cargo:rustc-link-lib=static=settings");
    println!("cargo:rustc-link-lib=static=rustbindsettings");
}
