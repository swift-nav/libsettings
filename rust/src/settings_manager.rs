extern crate yaml_rust;
use yaml_rust::{Yaml, YamlLoader};

use std::sync::Mutex;

const _SOURCE: &'static str = include_str!("../../settings.yaml");

#[derive(PartialEq, Debug)]
pub enum SettingType {
    StInteger,
    StBoolean,
    StDouble,
    StString,
    StEnum,
}

lazy_static! {
    static ref SETTINGS: Mutex<Yaml> = {
        let y = YamlLoader::load_from_str(_SOURCE);
        let docs = y.expect("Could not parse settings.yaml");
        let mut the_doc: Option<Yaml> = None;
        for elem in docs {
            the_doc = Some(elem);
            break;
        }
        return Mutex::new(the_doc.expect("No root document in settings.yaml"));
    };
}

pub fn lookup_setting_type(section: &str, name_in: &str) -> Option<SettingType> {
    let settings = &*SETTINGS.lock().unwrap();
    for setting in settings.as_vec().unwrap() {
        let setting = setting.as_hash().unwrap();
        let group = Yaml::from_str("group");
        if !setting.contains_key(&group) {
            continue;
        }
        let group = setting[&group].as_str().unwrap();
        let name = Yaml::from_str("name");
        if !setting.contains_key(&name) {
            continue;
        }
        let name = setting[&name].as_str().unwrap();
        let type_ = Yaml::from_str("type");
        if !setting.contains_key(&type_) {
            continue;
        }
        let type_ = setting[&type_].as_str().unwrap();
        if group == section && name == name_in {
            let lower_type = type_.to_lowercase();
            if lower_type == "integer" {
                return Some(SettingType::StInteger);
            } else if lower_type == "boolean" {
                return Some(SettingType::StBoolean);
            } else if lower_type == "double" {
                return Some(SettingType::StDouble);
            } else if lower_type == "enum" {
                return Some(SettingType::StEnum);
            } else if lower_type == "string" {
                return Some(SettingType::StString);
            }
            break;
        }
    }
    return None;
}

#[test]
fn test_lookup_setting_type() {
    assert_eq!(
        lookup_setting_type("solution", "soln_freq"),
        Some(SettingType::StInteger)
    );
    assert_eq!(
        lookup_setting_type("tcp_server0", "enabled_sbp_messages"),
        Some(SettingType::StString)
    );
    assert_eq!(lookup_setting_type("solution", "froo_froo"), None);
}
