use std::path::PathBuf;
use std::{env, fs};

use walkdir::{DirEntry, WalkDir};

fn main() {
    println!("cargo:rerun-if-changed=build.rs");

    if libsettings_dir().exists() {
        fs::remove_dir_all(libsettings_dir()).expect("failed to clean up libsettings source");
    }
    copy_libsettings_src().expect("failed to copy libsettings source");

    let out_dir: PathBuf = env::var("OUT_DIR").expect("OUT_DIR was not set").into();
    let dst = cmake::Config::new(libsettings_dir())
        .define("SKIP_UNIT_TESTS", "ON")
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("libsettings_ENABLE_PYTHON", "OFF")
        .define("CMAKE_INSTALL_PREFIX", &out_dir)
        .build();

    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:rustc-link-lib=static=sbp");
    println!("cargo:rustc-link-lib=static=settings");
    println!("cargo:rustc-link-lib=static=swiftnav");

    let bindings = bindgen::Builder::default()
        .header("./libsettings_wrapper.h")
        .allowlist_function("settings_create")
        .allowlist_function("settings_destroy")
        .allowlist_function("settings_read_bool")
        .allowlist_function("settings_read_by_idx")
        .allowlist_function("settings_read_float")
        .allowlist_function("settings_read_int")
        .allowlist_function("settings_read_str")
        .allowlist_function("settings_write_str")
        .allowlist_type("settings_t")
        .allowlist_type("sbp_msg_callback_t")
        .allowlist_type("sbp_msg_callbacks_node_t")
        .allowlist_type("settings_api_t")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_MODIFY_DISABLED")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_OK")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_PARSE_FAILED")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_READ_ONLY")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_SERVICE_FAILED")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_SETTING_REJECTED")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_TIMEOUT")
        .allowlist_type("settings_write_res_e_SETTINGS_WR_VALUE_REJECTED")
        .clang_arg(format!("-I{}/include", dst.display()))
        .clang_arg(format!(
            "-I{}/third_party/libswiftnav/include",
            dst.display()
        ))
        .clang_arg(format!("-I{}/third_party/libsbp/c/include", dst.display()))
        .generate()
        .unwrap();

    bindings.write_to_file(out_dir.join("bindings.rs")).unwrap()
}

fn copy_libsettings_src() -> Result<(), std::io::Error> {
    const FOLDERS: &[&'static str] = &["cmake", "include", "src", "third_party"];
    const FILES: &[&'static str] = &["CMakeLists.txt"];

    let root_dir = project_root();
    let root_len = root_dir.as_path().components().count();
    let output_dir = libsettings_dir();

    for folder in FOLDERS {
        let dir = root_dir.join(folder);
        for entry in WalkDir::new(&dir).into_iter().filter_entry(not_hidden) {
            let entry = entry?;
            let src: PathBuf = entry.path().components().skip(root_len).collect();
            let dest = if src.components().count() == 0 {
                output_dir.clone()
            } else {
                output_dir.join(&src)
            };
            if entry.file_type().is_dir() {
                if fs::metadata(&dest).is_err() {
                    fs::create_dir_all(&dest)?;
                }
            } else {
                fs::copy(entry.path(), dest)?;
            }
        }
    }

    for file in FILES {
        let src = root_dir.join(file);
        let dest = output_dir.join(file);
        fs::copy(src, dest)?;
    }

    Ok(())
}

fn project_root() -> PathBuf {
    let manifest = manifest_dir();
    let root = manifest
        .ancestors()
        .skip(2)
        .next()
        .expect("could not find project root");
    // CARGO_MANIFEST_DIR` includes the `target` dir when invoked from
    // cargo package, so remove that
    if root.ends_with("target") {
        root.parent().unwrap().to_owned()
    } else {
        root.to_owned()
    }
}

fn manifest_dir() -> PathBuf {
    env::var("CARGO_MANIFEST_DIR")
        .expect("CARGO_MANIFEST_DIR was not set")
        .into()
}

fn libsettings_dir() -> PathBuf {
    manifest_dir().join("src").join("libsettings")
}

fn not_hidden(entry: &DirEntry) -> bool {
    entry
        .file_name()
        .to_str()
        .map(|s| !s.starts_with("."))
        .unwrap_or(true)
}
