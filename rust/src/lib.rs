mod client;
mod settings;

pub use client::{Client, Error, ReadSettingError, WriteSettingError};
pub use settings::{lookup_setting, settings, Setting, SettingKind, SettingValue};
