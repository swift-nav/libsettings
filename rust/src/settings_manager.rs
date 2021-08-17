use std::fmt;

use serde::{
    de::{self, Unexpected},
    Deserialize, Deserializer,
};

lazy_static::lazy_static! {
    static ref SETTINGS: Vec<Setting> = {
        serde_yaml::from_str(include_str!("../../settings.yaml"))
            .expect("Could not parse settings.yaml")
    };
}

#[derive(Debug, Clone, PartialEq, Deserialize)]
pub struct Setting {
    pub name: String,

    pub group: String,

    #[serde(rename = "type")]
    pub kind: SettingKind,

    #[serde(deserialize_with = "deserialize_bool", default)]
    pub expert: bool,

    #[serde(deserialize_with = "deserialize_bool", default)]
    pub readonly: bool,

    #[serde(rename = "Description")]
    pub description: Option<String>,

    #[serde(
        alias = "default value",
        deserialize_with = "deserialize_string",
        default
    )]
    pub default_value: Option<String>,

    #[serde(rename = "Notes")]
    pub notes: Option<String>,

    #[serde(deserialize_with = "deserialize_string", default)]
    pub units: Option<String>,

    #[serde(
        rename = "enumerated possible values",
        deserialize_with = "deserialize_string",
        default
    )]
    pub enumerated_possible_values: Option<String>,

    pub digits: Option<String>,
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

pub fn lookup_setting(group: impl AsRef<str>, name: impl AsRef<str>) -> Option<&'static Setting> {
    let group = group.as_ref();
    let name = name.as_ref();
    SETTINGS.iter().find(|s| s.group == group && s.name == name)
}

fn deserialize_bool<'de, D>(deserializer: D) -> Result<bool, D::Error>
where
    D: Deserializer<'de>,
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

        fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            match v {
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

fn deserialize_string<'de, D>(deserializer: D) -> Result<Option<String>, D::Error>
where
    D: Deserializer<'de>,
{
    struct StringVisitor;

    impl<'de> de::Visitor<'de> for StringVisitor {
        type Value = Option<String>;

        fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
            formatter.write_str("an optional string")
        }

        fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            match v {
                "N/A" | "" => Ok(None),
                _ => Ok(Some(v.to_owned())),
            }
        }

        fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            Ok(Some(v.to_string()))
        }

        fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            Ok(Some(v.to_string()))
        }

        fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            Ok(Some(v.to_string()))
        }

        fn visit_unit<E>(self) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            Ok(None)
        }
    }

    deserializer.deserialize_any(StringVisitor)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lookup_setting() {
        assert_eq!(
            lookup_setting("solution", "soln_freq"),
            Some(&Setting {
                name: "soln_freq".into(),
                group: "solution".into(),
                kind: SettingKind::Integer,
                readonly: false,
                expert: false,
                units: Some("Hz".into()),
                default_value: Some("10".into()),
                description: Some("The frequency at which a position solution is computed.".into()),
                notes: None,
                enumerated_possible_values: None,
                digits: None,
            })
        );

        assert_eq!(lookup_setting("solution", "froo_froo"), None);
    }

    #[test]
    fn test_na_is_none() {
        let setting = lookup_setting("tcp_server0", "enabled_sbp_messages").unwrap();
        assert_eq!(setting.units, None);
    }
}
