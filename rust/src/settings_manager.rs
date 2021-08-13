use std::fmt;

use serde::{
    de::{self, Unexpected},
    Deserialize,
};

lazy_static::lazy_static! {
    static ref SETTINGS: Vec<Setting> = {
        serde_yaml::from_str(include_str!("../../settings.yaml"))
            .expect("Could not parse settings.yaml")
    };
}

#[derive(Debug, Clone, Deserialize)]
pub struct Setting {
    name: String,

    group: String,

    #[serde(rename = "type")]
    kind: SettingKind,

    #[serde(deserialize_with = "deserialize_bool", default)]
    expert: bool,

    #[serde(deserialize_with = "deserialize_bool", default)]
    readonly: bool,

    #[serde(rename = "Description")]
    description: Option<String>,

    #[serde(alias = "default value")]
    default_value: Option<String>,

    #[serde(rename = "Notes")]
    notes: Option<String>,

    units: Option<String>,

    #[serde(rename = "enumerated possible values")]
    enumerated_possible_values: Option<String>,

    digits: Option<String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Deserialize)]
pub enum SettingKind {
    #[serde(rename = "integer", alias = "int")]
    Integer,

    #[serde(rename = "boolean", alias = "bool")]
    Boolean,

    #[serde(rename = "float")]
    Float,

    #[serde(rename = "double", alias = "Double")]
    Double,

    #[serde(rename = "string")]
    String,

    #[serde(rename = "enum")]
    Enum,

    #[serde(rename = "packed bitfield")]
    PackedBitfield,
}

pub fn lookup_setting(group: &str, name: &str) -> Option<&'static Setting> {
    SETTINGS.iter().find(|s| s.group == group && s.name == name)
}

pub fn lookup_setting_kind(group: &str, name: &str) -> Option<SettingKind> {
    lookup_setting(group, name).map(|s| s.kind)
}

fn deserialize_bool<'de, D>(deserializer: D) -> Result<bool, D::Error>
where
    D: de::Deserializer<'de>,
{
    struct BoolVisitor;

    impl<'de> de::Visitor<'de> for BoolVisitor {
        type Value = bool;

        fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
            formatter.write_str("a bool or a string containing a bool")
        }

        fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            Ok(v)
        }

        fn visit_str<E>(self, s: &str) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            match s {
                "True" | "true" => Ok(true),
                "False" | "false" => Ok(false),
                other => Err(de::Error::invalid_value(
                    Unexpected::Str(other),
                    &"True or False",
                )),
            }
        }
    }

    deserializer.deserialize_any(BoolVisitor)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lookup_setting_kind() {
        assert_eq!(
            lookup_setting_kind("solution", "soln_freq"),
            Some(SettingKind::Integer)
        );
        assert_eq!(
            lookup_setting_kind("tcp_server0", "enabled_sbp_messages"),
            Some(SettingKind::String)
        );
        assert_eq!(lookup_setting_kind("solution", "froo_froo"), None);
    }
}
