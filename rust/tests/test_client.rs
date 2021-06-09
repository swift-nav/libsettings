#[cfg(feature = "tests")]
use std::thread;

use std::boxed::Box;

use sbp::messages::settings::{MsgSettingsReadReq, MsgSettingsReadResp};
use sbp::messages::SBPMessage;
use sbp::SbpString;

use libsettings::client::{read_setting, SettingValue};

#[test]
fn mock_read_setting_int() {
    let (group, name) = ("sbp", "obs_msg_max_size");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };
    let mut c = request_msg.to_frame().unwrap();
    let mut expect_buf: Vec<u8> = Vec::new();
    expect_buf.append(&mut c);

    stream.wait_for(&expect_buf);

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\010\0", group, name).to_string()),
    };
    let mut buf: Vec<u8> = Vec::new();

    let mut f = reply_msg.to_frame().unwrap();
    buf.append(&mut f);

    stream.push_bytes_to_read(&buf);

    let stream_write = stream.clone();
    let response = read_setting(
        Box::new(stream),
        Box::new(stream_write),
        group.to_string(),
        name.to_string(),
    );

    assert_eq!(response, SettingValue::integer(10));
}

#[test]
fn mock_read_setting_bool() {
    let (group, name) = ("surveyed_position", "broadcast");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };
    let mut c = request_msg.to_frame().unwrap();
    let mut expect_buf: Vec<u8> = Vec::new();
    expect_buf.append(&mut c);

    stream.wait_for(&expect_buf);

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0True\0", group, name).to_string()),
    };
    let mut buf: Vec<u8> = Vec::new();

    let mut f = reply_msg.to_frame().unwrap();
    buf.append(&mut f);

    stream.push_bytes_to_read(&buf);

    let stream_write = stream.clone();
    let response = read_setting(
        Box::new(stream),
        Box::new(stream_write),
        group.to_string(),
        name.to_string(),
    );

    assert_eq!(response, SettingValue::boolean(true));
}

#[test]
fn mock_read_setting_double() {
    let (group, name) = ("surveyed_position", "surveyed_lat");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };
    let mut c = request_msg.to_frame().unwrap();
    let mut expect_buf: Vec<u8> = Vec::new();
    expect_buf.append(&mut c);

    stream.wait_for(&expect_buf);

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\00.1\0", group, name).to_string()),
    };
    let mut buf: Vec<u8> = Vec::new();

    let mut f = reply_msg.to_frame().unwrap();
    buf.append(&mut f);

    stream.push_bytes_to_read(&buf);

    let stream_write = stream.clone();
    let response = read_setting(
        Box::new(stream),
        Box::new(stream_write),
        group.to_string(),
        name.to_string(),
    );

    assert_eq!(response, SettingValue::double(0.1_f32));
}

#[test]
fn mock_read_setting_string() {
    let (group, name) = ("rtcm_out", "ant_descriptor");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };
    let mut c = request_msg.to_frame().unwrap();
    let mut expect_buf: Vec<u8> = Vec::new();
    expect_buf.append(&mut c);

    stream.wait_for(&expect_buf);

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0foo\0", group, name).to_string()),
    };
    let mut buf: Vec<u8> = Vec::new();

    let mut f = reply_msg.to_frame().unwrap();
    buf.append(&mut f);

    stream.push_bytes_to_read(&buf);

    let stream_write = stream.clone();
    let response = read_setting(
        Box::new(stream),
        Box::new(stream_write),
        group.to_string(),
        name.to_string(),
    );

    assert_eq!(response, SettingValue::string(Box::new("foo".to_string())));
}

#[test]
fn mock_read_setting_enum() {
    let (group, name) = ("frontend", "antenna_selection");
    let mut stream = mockstream::SyncMockStream::new();

    let request_msg = MsgSettingsReadReq {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
    };
    let mut c = request_msg.to_frame().unwrap();
    let mut expect_buf: Vec<u8> = Vec::new();
    expect_buf.append(&mut c);

    stream.wait_for(&expect_buf);

    let reply_msg = MsgSettingsReadResp {
        sender_id: Some(0x42),
        setting: SbpString::from(format!("{}\0{}\0Secondary\0", group, name).to_string()),
    };
    let mut buf: Vec<u8> = Vec::new();

    let mut f = reply_msg.to_frame().unwrap();
    buf.append(&mut f);

    stream.push_bytes_to_read(&buf);

    let stream_write = stream.clone();
    let response = read_setting(
        Box::new(stream),
        Box::new(stream_write),
        group.to_string(),
        name.to_string(),
    );

    assert_eq!(
        response,
        SettingValue::string(Box::new("Secondary".to_string()))
    );
}
