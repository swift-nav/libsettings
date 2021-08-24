use sbp::messages::settings::{MsgSettingsReadReq, MsgSettingsReadResp};
use sbp::messages::SBPMessage;
use sbp::SbpString;

use libsettings::client::{Client, SettingValue};

static SETTINGS_SENDER_ID: u16 = 0x42;

#[test]
fn mock_read_setting_int() {
    let (group, name) = ("sbp", "obs_msg_max_size");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(SETTINGS_SENDER_ID),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };

    stream.wait_for(&request_msg.to_frame().unwrap().to_vec());

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\010\0", group, name).to_string()),
    };

    stream.push_bytes_to_read(&reply_msg.to_frame().unwrap().to_vec());

    let mut client = Client::new(stream.clone(), stream.clone());
    let response = client.read_setting(group, name);
    assert_eq!(response, SettingValue::Integer(10));

    stream.wait_for(&request_msg.to_frame().unwrap().to_vec());
    stream.push_bytes_to_read(&reply_msg.to_frame().unwrap().to_vec());

    let response = client.read_setting(group, name);
    assert_eq!(response, SettingValue::Integer(10));
}

#[test]
fn mock_read_setting_bool() {
    let (group, name) = ("surveyed_position", "broadcast");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(SETTINGS_SENDER_ID),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };

    stream.wait_for(&request_msg.to_frame().unwrap().to_vec());

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0True\0", group, name).to_string()),
    };

    stream.push_bytes_to_read(&reply_msg.to_frame().unwrap().to_vec());

    let mut client = Client::new(stream.clone(), stream);
    let response = client.read_setting(group, name);

    assert_eq!(response, SettingValue::Boolean(true));
}

#[test]
fn mock_read_setting_double() {
    let (group, name) = ("surveyed_position", "surveyed_lat");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(SETTINGS_SENDER_ID),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };

    stream.wait_for(&request_msg.to_frame().unwrap().to_vec());

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(SETTINGS_SENDER_ID),
        setting: SbpString::from(format!("{}\0{}\00.1\0", group, name).to_string()),
    };

    stream.push_bytes_to_read(&reply_msg.to_frame().unwrap().to_vec());

    let mut client = Client::new(stream.clone(), stream);
    let response = client.read_setting(group, name);

    assert_eq!(response, SettingValue::Float(0.1_f32));
}

#[test]
fn mock_read_setting_string() {
    let (group, name) = ("rtcm_out", "ant_descriptor");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };

    stream.wait_for(&request_msg.to_frame().unwrap().to_vec());

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(SETTINGS_SENDER_ID),
        setting: SbpString::from(format!("{}\0{}\0foo\0", group, name).to_string()),
    };

    stream.push_bytes_to_read(&reply_msg.to_frame().unwrap().to_vec());

    let mut client = Client::new(stream.clone(), stream);
    let response = client.read_setting(group, name);

    assert_eq!(response, SettingValue::String("foo".to_string()));
}

#[test]
fn mock_read_setting_enum() {
    let (group, name) = ("frontend", "antenna_selection");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(SETTINGS_SENDER_ID),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };

    stream.wait_for(&request_msg.to_frame().unwrap().to_vec());

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0Secondary\0", group, name).to_string()),
    };

    stream.push_bytes_to_read(&reply_msg.to_frame().unwrap().to_vec());

    let mut client = Client::new(stream.clone(), stream);
    let response = client.read_setting(group, name);

    assert_eq!(response, SettingValue::String("Secondary".to_string()));
}
