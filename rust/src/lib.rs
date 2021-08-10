pub mod client;
pub mod settings_manager;

#[allow(warnings, unused)]
pub(crate) mod bindings {
    include!(concat!(env!("OUT_DIR"), "/libsettings.rs"));
}
