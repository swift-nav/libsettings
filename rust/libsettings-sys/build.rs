fn main() {
    let dst = cmake::Config::new("../..")
        .define("SKIP_UNIT_TESTS", "ON")
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("libsettings_ENABLE_PYTHON", "OFF")
        .build();
    println!("cargo:rustc-link-search=native={}/lib", dst.display());

    // let dst = cmake::Config::new(".").build();
    // println!("cargo:rustc-link-search=native={}/lib", dst.display());

    println!("cargo:rustc-link-lib=static=sbp");
    println!("cargo:rustc-link-lib=static=settings");
    println!("cargo:rustc-link-lib=static=swiftnav");
    // println!("cargo:rustc-link-lib=static=rustbindsettings");

    // let bindings = bindgen::Builder::default()
    //     .header("./libsettings_wrapper.h")
    //     .allowlist_function("settings_create")
    //     .allowlist_function("settings_destroy")
    //     .allowlist_function("settings_read_bool")
    //     .allowlist_function("settings_read_by_idx")
    //     .allowlist_function("settings_read_float")
    //     .allowlist_function("settings_read_int")
    //     .allowlist_function("settings_read_str")
    //     .allowlist_function("settings_write_str")
    //     .allowlist_type("settings_t")
    //     .allowlist_type("sbp_msg_callback_t")
    //     .allowlist_type("sbp_msg_callbacks_node_t")
    //     .allowlist_type("settings_api_t")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_MODIFY_DISABLED")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_OK")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_PARSE_FAILED")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_READ_ONLY")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_SERVICE_FAILED")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_SETTING_REJECTED")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_TIMEOUT")
    //     .allowlist_type("settings_write_res_e_SETTINGS_WR_VALUE_REJECTED")
    //     .clang_arg("-I../../include")
    //     .clang_arg("-I../../third_party/libswiftnav/include")
    //     .clang_arg("-I../../third_party/libsbp/c/include")
    //     .generate()
    //     .expect("Unable to generate bindings");

    // // Write out the generated bindings...
    // let out_dir = std::env::var("OUT_DIR").unwrap();
    // let out_dir = std::path::PathBuf::from(out_dir);

    // bindings
    //     .write_to_file(out_dir.join("bindings.rs"))
    //     .expect("Couldn't write bindings!");
}
