use std::{
    convert::TryInto,
    ffi::{self, CStr, CString},
    os::raw::{c_char, c_void},
    ptr, slice,
    time::Duration,
};

use crossbeam_utils::thread;
use log::{debug, error, warn};
use sbp::{
    link::{Key, Link},
    Sbp, SbpMessage,
};
use sbp_settings_sys::{
    sbp_msg_callback_t, sbp_msg_callbacks_node_t, settings_api_t, settings_create,
    settings_destroy, settings_read_bool, settings_read_by_idx, settings_read_float,
    settings_read_int, settings_read_str, settings_t,
    settings_write_res_e_SETTINGS_WR_MODIFY_DISABLED, settings_write_res_e_SETTINGS_WR_OK,
    settings_write_res_e_SETTINGS_WR_PARSE_FAILED, settings_write_res_e_SETTINGS_WR_READ_ONLY,
    settings_write_res_e_SETTINGS_WR_SERVICE_FAILED,
    settings_write_res_e_SETTINGS_WR_SETTING_REJECTED, settings_write_res_e_SETTINGS_WR_TIMEOUT,
    settings_write_res_e_SETTINGS_WR_VALUE_REJECTED, settings_write_str, size_t,
};

use crate::{settings, SettingKind, SettingValue};

const SENDER_ID: u16 = 0x42;

pub struct Client<'a> {
    inner: Box<ClientInner<'a>>,
}

struct ClientInner<'a> {
    context: *mut Context<'a>,
    api: *mut settings_api_t,
    ctx: *mut settings_t,
}

impl Drop for ClientInner<'_> {
    fn drop(&mut self) {
        unsafe {
            // Safety: Created via Box::into_raw
            let _ = Box::from_raw(self.context);
            // Safety: Created via Box::into_raw
            let _ = Box::from_raw(self.api);
            settings_destroy(&mut self.ctx);
        }
    }
}

unsafe impl Send for ClientInner<'_> {}
unsafe impl Sync for ClientInner<'_> {}

impl<'a> Client<'a> {
    pub fn new<F>(link: Link<'a, ()>, sender: F) -> Client<'a>
    where
        F: FnMut(Sbp) -> Result<(), Box<dyn std::error::Error + Send + Sync>> + 'static,
    {
        let context = Box::new(Context {
            link,
            sender: Box::new(sender),
            callbacks: Vec::new(),
            lock: Lock::new(),
            event: None,
        });

        let api = Box::new(settings_api_t {
            ctx: ptr::null_mut(),
            send: Some(libsettings_send),
            send_from: Some(libsettings_send_from),
            wait_init: Some(libsettings_wait_init),
            wait: Some(libsettings_wait),
            wait_deinit: None,
            signal: Some(libsettings_signal),
            wait_thd: Some(libsettings_wait_thd),
            signal_thd: Some(libsettings_signal_thd),
            lock: Some(libsettings_lock),
            unlock: Some(libsettings_unlock),
            register_cb: Some(register_cb),
            unregister_cb: Some(unregister_cb),
            log: None,
            log_preformat: false,
        });

        let mut inner = Box::new(ClientInner {
            context: ptr::null_mut(),
            api: Box::into_raw(api),
            ctx: ptr::null_mut(),
        });

        let context_raw = Box::into_raw(context);
        inner.context = context_raw;

        // Safety: inner.api was just created via Box::into_raw so it is
        // properly aligned and non-null
        unsafe {
            (*inner.api).ctx = context_raw as *mut c_void;
        }

        inner.ctx = unsafe { settings_create(SENDER_ID, inner.api) };
        assert!(!inner.ctx.is_null());

        Client { inner }
    }

    pub fn read_all(&self) -> Vec<Result<ReadByIdxResult, Error<ReadSettingError>>> {
        const NUM_WORKERS: usize = 5;

        thread::scope(move |scope| {
            let (idx_s, idx_r) = crossbeam_channel::bounded(NUM_WORKERS);
            let (results_s, results_r) = crossbeam_channel::unbounded();

            scope.spawn(move |_| {
                let mut idx = 0;
                while idx_s.send(idx).is_ok() {
                    idx += 1;
                }
            });

            for _ in 0..NUM_WORKERS {
                let idx_r = idx_r.clone();
                let results_s = results_s.clone();
                scope.spawn(move |_| {
                    for idx in idx_r.iter() {
                        match self.read_by_index(idx).map_err(Error::Err).transpose() {
                            Some(res) => results_s.send(res).expect("results channel closed"),
                            None => break,
                        }
                    }
                });
            }

            drop(results_s);
            results_r.iter().collect::<Vec<_>>()
        })
        .expect("read_all worker thread panicked")
    }

    pub fn read_by_index(&self, idx: u16) -> Result<Option<ReadByIdxResult>, ReadSettingError> {
        const BUF_SIZE: usize = 255;

        let mut section = Vec::<c_char>::with_capacity(BUF_SIZE);
        let mut name = Vec::<c_char>::with_capacity(BUF_SIZE);
        let mut value = Vec::<c_char>::with_capacity(BUF_SIZE);
        let mut fmt_type = Vec::<c_char>::with_capacity(BUF_SIZE);
        let mut event = Event::new();

        let status = unsafe {
            settings_read_by_idx(
                self.inner.ctx,
                &mut event as *mut Event as *mut _,
                idx,
                section.as_mut_ptr(),
                BUF_SIZE as size_t,
                name.as_mut_ptr(),
                BUF_SIZE as size_t,
                value.as_mut_ptr(),
                BUF_SIZE as size_t,
                fmt_type.as_mut_ptr(),
                BUF_SIZE as size_t,
            )
        };

        match status {
            0 => Ok(Some(unsafe {
                ReadByIdxResult {
                    group: CStr::from_ptr(section.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    name: CStr::from_ptr(name.as_ptr()).to_string_lossy().into_owned(),
                    value: CStr::from_ptr(value.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                    fmt_type: CStr::from_ptr(fmt_type.as_ptr())
                        .to_string_lossy()
                        .into_owned(),
                }
            })),
            status if status > 0 => Ok(None),
            status => Err(status.into()),
        }
    }

    pub fn read_setting(
        &self,
        group: impl AsRef<str>,
        name: impl AsRef<str>,
    ) -> Option<Result<SettingValue, Error<ReadSettingError>>> {
        self.read_setting_inner(group.as_ref(), name.as_ref())
    }

    fn read_setting_inner(
        &self,
        group: &str,
        name: &str,
    ) -> Option<Result<SettingValue, Error<ReadSettingError>>> {
        let setting = settings()
            .iter()
            .find(|s| s.group == group && s.name == name)?;

        let cgroup = match CString::new(group) {
            Ok(cgroup) => cgroup,
            Err(e) => return Some(Err(e.into())),
        };

        let cname = match CString::new(name) {
            Ok(cname) => cname,
            Err(e) => return Some(Err(e.into())),
        };

        let (res, status) = match setting.kind {
            SettingKind::Integer => unsafe {
                let mut value: i32 = 0;
                let status =
                    settings_read_int(self.inner.ctx, cgroup.as_ptr(), cname.as_ptr(), &mut value);
                (SettingValue::Integer(value), status)
            },
            SettingKind::Boolean => unsafe {
                let mut value: bool = false;
                let status =
                    settings_read_bool(self.inner.ctx, cgroup.as_ptr(), cname.as_ptr(), &mut value);
                (SettingValue::Boolean(value), status)
            },
            SettingKind::Float | SettingKind::Double => unsafe {
                let mut value: f32 = 0.0;
                let status = settings_read_float(
                    self.inner.ctx,
                    cgroup.as_ptr(),
                    cname.as_ptr(),
                    &mut value,
                );
                (SettingValue::Float(value), status)
            },
            SettingKind::String | SettingKind::Enum | SettingKind::PackedBitfield => unsafe {
                let mut buf = Vec::<c_char>::with_capacity(1024);
                let status = settings_read_str(
                    self.inner.ctx,
                    cgroup.as_ptr(),
                    cname.as_ptr(),
                    buf.as_mut_ptr(),
                    buf.capacity().try_into().unwrap(),
                );
                (
                    SettingValue::String(
                        CStr::from_ptr(buf.as_ptr()).to_string_lossy().into_owned(),
                    ),
                    status,
                )
            },
        };

        debug!("{} {} {:?} {}", setting.group, setting.name, res, status);

        if status == 0 {
            Some(Ok(res))
        } else {
            Some(Err(status.into()))
        }
    }

    pub fn write_setting(
        &self,
        group: impl AsRef<str>,
        name: impl AsRef<str>,
        value: impl AsRef<str>,
    ) -> Result<(), Error<WriteSettingError>> {
        self.write_setting_inner(group.as_ref(), name.as_ref(), value.as_ref())
    }

    fn write_setting_inner(
        &self,
        group: &str,
        name: &str,
        value: &str,
    ) -> Result<(), Error<WriteSettingError>> {
        let cgroup = CString::new(group)?;
        let cname = CString::new(name)?;
        let cvalue = CString::new(value)?;
        let mut event = Event::new();

        let result = unsafe {
            settings_write_str(
                self.inner.ctx,
                &mut event as *mut Event as *mut _,
                cgroup.as_ptr(),
                cname.as_ptr(),
                cvalue.as_ptr(),
            )
        };

        #[allow(non_upper_case_globals)]
        match result {
            settings_write_res_e_SETTINGS_WR_OK => Ok(()),
            code => Err(code.into()),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Error<E> {
    NulError,
    Err(E),
}

impl<E> From<ffi::NulError> for Error<E> {
    fn from(_: ffi::NulError) -> Self {
        Error::NulError
    }
}

impl<E> From<u32> for Error<E>
where
    E: From<u32>,
{
    fn from(err: u32) -> Self {
        Error::Err(err.into())
    }
}

impl<E> From<i32> for Error<E>
where
    E: From<i32>,
{
    fn from(err: i32) -> Self {
        Error::Err(err.into())
    }
}

impl<E> std::fmt::Display for Error<E>
where
    E: std::error::Error,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::NulError => write!(f, "error converting to CString"),
            Error::Err(err) => write!(f, "{}", err),
        }
    }
}

impl<E> std::error::Error for Error<E> where E: std::error::Error {}

#[derive(Debug, Clone)]
pub struct ReadByIdxResult {
    pub group: String,
    pub name: String,
    pub value: String,
    pub fmt_type: String,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WriteSettingError {
    ValueRejected,
    SettingRejected,
    ParseFailed,
    ReadOnly,
    ModifyDisabled,
    ServiceFailed,
    Timeout,
    Unknown,
}

impl From<u32> for WriteSettingError {
    fn from(n: u32) -> Self {
        #[cfg(target_os = "windows")]
        let n = n as i32;

        #[allow(non_upper_case_globals)]
        match n {
            settings_write_res_e_SETTINGS_WR_VALUE_REJECTED => WriteSettingError::ValueRejected,
            settings_write_res_e_SETTINGS_WR_SETTING_REJECTED => WriteSettingError::SettingRejected,
            settings_write_res_e_SETTINGS_WR_PARSE_FAILED => WriteSettingError::ParseFailed,
            settings_write_res_e_SETTINGS_WR_READ_ONLY => WriteSettingError::ReadOnly,
            settings_write_res_e_SETTINGS_WR_MODIFY_DISABLED => WriteSettingError::ModifyDisabled,
            settings_write_res_e_SETTINGS_WR_SERVICE_FAILED => WriteSettingError::ServiceFailed,
            settings_write_res_e_SETTINGS_WR_TIMEOUT => WriteSettingError::Timeout,
            _ => WriteSettingError::Unknown,
        }
    }
}

impl From<i32> for WriteSettingError {
    fn from(n: i32) -> Self {
        (n as u32).into()
    }
}

impl std::fmt::Display for WriteSettingError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            WriteSettingError::ValueRejected => {
                write!(f, "setting value invalid",)
            }
            WriteSettingError::SettingRejected => {
                write!(f, "setting does not exist")
            }
            WriteSettingError::ParseFailed => {
                write!(f, "could not parse setting value ")
            }
            WriteSettingError::ReadOnly => {
                write!(f, "setting is read only")
            }
            WriteSettingError::ModifyDisabled => {
                write!(f, "setting is not modifiable")
            }
            WriteSettingError::ServiceFailed => write!(f, "system failure during setting"),
            WriteSettingError::Timeout => write!(f, "request wasn't replied in time"),
            WriteSettingError::Unknown => {
                write!(f, "unknown settings write response")
            }
        }
    }
}

impl std::error::Error for WriteSettingError {}

#[derive(Debug, Clone, Copy)]
pub struct ReadSettingError {
    code: u32,
}

impl From<u32> for ReadSettingError {
    fn from(code: u32) -> Self {
        Self { code }
    }
}

impl From<i32> for ReadSettingError {
    fn from(code: i32) -> Self {
        (code as u32).into()
    }
}

impl std::fmt::Display for ReadSettingError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "settings read failed with status code {}", self.code)
    }
}

impl std::error::Error for ReadSettingError {}

type SenderFunc = Box<dyn FnMut(Sbp) -> Result<(), Box<dyn std::error::Error + Send + Sync>>>;

#[repr(C)]
struct Context<'a> {
    link: Link<'a, ()>,
    sender: SenderFunc,
    callbacks: Vec<Callback>,
    event: Option<Event>,
    lock: Lock,
}

impl Context<'_> {
    fn callback_broker(&self, msg: Sbp) {
        let cb_data = {
            let _guard = self.lock.lock();
            let idx = if let Some(idx) = self
                .callbacks
                .iter()
                .position(|cb| cb.msg_type == msg.message_type())
            {
                idx
            } else {
                error!(
                    "callback not registered for message type {}",
                    msg.message_type()
                );
                return;
            };
            self.callbacks[idx]
        };
        let mut frame = match sbp::to_vec(&msg) {
            Ok(frame) => frame,
            Err(e) => {
                error!("failed to frame sbp message {}", e);
                return;
            }
        };
        let frame_len = frame.len();
        let payload = &mut frame[sbp::HEADER_LEN..frame_len - sbp::CRC_LEN];
        let cb = match cb_data.cb {
            Some(cb) => cb,
            None => {
                error!("provided callback was none");
                return;
            }
        };
        let cb_context = cb_data.cb_context;
        unsafe {
            cb(
                msg.sender_id().unwrap_or(0),
                payload.len() as u8,
                payload.as_mut_ptr(),
                cb_context,
            )
        };
    }
}

unsafe impl Send for Context<'_> {}
unsafe impl Sync for Context<'_> {}

#[repr(C)]
#[derive(Clone, Copy)]
struct Callback {
    node: usize,
    msg_type: u16,
    cb: sbp_msg_callback_t,
    cb_context: *mut c_void,
    key: Key,
}

#[repr(C)]
struct Event {
    condvar: parking_lot::Condvar,
    lock: parking_lot::Mutex<()>,
}

impl Event {
    fn new() -> Self {
        Self {
            condvar: parking_lot::Condvar::new(),
            lock: parking_lot::Mutex::new(()),
        }
    }

    fn wait_timeout(&self, ms: i32) -> bool {
        let mut started = self.lock.lock();
        let result = self
            .condvar
            .wait_for(&mut started, Duration::from_millis(ms as u64));
        !result.timed_out()
    }

    fn set(&self) {
        let _ = self.lock.lock();
        if !self.condvar.notify_one() {
            warn!("event set did not notify anything");
        }
    }
}

#[repr(C)]
struct Lock(parking_lot::Mutex<()>);

impl Lock {
    fn new() -> Self {
        Self(parking_lot::Mutex::new(()))
    }

    fn lock(&self) -> parking_lot::MutexGuard<()> {
        self.0.lock()
    }

    fn acquire(&self) {
        std::mem::forget(self.0.lock());
    }

    fn release(&self) {
        // Safety: Only called via libsettings_unlock after libsettings_lock was called
        unsafe { self.0.force_unlock() }
    }
}

struct CtxPtr(*mut c_void);

unsafe impl Sync for CtxPtr {}
unsafe impl Send for CtxPtr {}

#[no_mangle]
/// Thread safety: Should be called with lock already held
unsafe extern "C" fn register_cb(
    ctx: *mut c_void,
    msg_type: u16,
    cb: sbp_msg_callback_t,
    cb_context: *mut c_void,
    node: *mut *mut sbp_msg_callbacks_node_t,
) -> i32 {
    let context: &mut Context = &mut *(ctx as *mut _);
    let ctx_ptr = CtxPtr(ctx);
    let key = context.link.register_by_id(&[msg_type], move |msg: Sbp| {
        let context: &mut Context = &mut *(ctx_ptr.0 as *mut _);
        context.callback_broker(msg)
    });
    context.callbacks.push(Callback {
        node: node as usize,
        msg_type,
        cb,
        cb_context,
        key,
    });
    0
}

#[no_mangle]
/// Thread safety: Should be called with lock already held
unsafe extern "C" fn unregister_cb(
    ctx: *mut c_void,
    node: *mut *mut sbp_msg_callbacks_node_t,
) -> i32 {
    let context: &mut Context = &mut *(ctx as *mut _);
    if (node as i32) == 0 {
        return -127;
    }
    let idx = match context
        .callbacks
        .iter()
        .position(|cb| cb.node == node as usize)
    {
        Some(idx) => idx,
        None => return -127,
    };
    let key = context.callbacks.remove(idx).key;
    context.link.unregister(key);
    0
}

#[no_mangle]
unsafe extern "C" fn libsettings_send(
    ctx: *mut c_void,
    msg_type: u16,
    len: u8,
    payload: *mut u8,
) -> i32 {
    libsettings_send_from(ctx, msg_type, len, payload, 0)
}

#[no_mangle]
unsafe extern "C" fn libsettings_send_from(
    ctx: *mut ::std::os::raw::c_void,
    msg_type: u16,
    len: u8,
    payload: *mut u8,
    sender_id: u16,
) -> i32 {
    let context: &mut Context = &mut *(ctx as *mut _);
    let msg = match Sbp::from_frame(sbp::Frame {
        msg_type,
        sender_id,
        payload: slice::from_raw_parts(payload, len as usize),
    }) {
        Ok(msg) => msg,
        Err(err) => {
            error!("error parsing message: {}", err);
            return -1;
        }
    };
    if let Err(err) = (context.sender)(msg) {
        error!("failed to send message: {}", err);
        return -1;
    };
    0
}

#[no_mangle]
extern "C" fn libsettings_wait(ctx: *mut c_void, timeout_ms: i32) -> i32 {
    assert!(timeout_ms > 0);
    let context: &mut Context = unsafe { &mut *(ctx as *mut _) };
    if context.event.as_ref().unwrap().wait_timeout(timeout_ms) {
        0
    } else {
        -1
    }
}

#[no_mangle]
extern "C" fn libsettings_wait_thd(event: *mut c_void, timeout_ms: i32) -> i32 {
    assert!(timeout_ms > 0);
    let event: &Event = unsafe { &*(event as *const _) };
    if event.wait_timeout(timeout_ms) {
        0
    } else {
        -1
    }
}

#[no_mangle]
extern "C" fn libsettings_signal_thd(event: *mut c_void) {
    let event: &Event = unsafe { &*(event as *const _) };
    event.set();
}

#[no_mangle]
extern "C" fn libsettings_signal(ctx: *mut c_void) {
    let context: &Context = unsafe { &*(ctx as *const _) };
    context.event.as_ref().unwrap().set();
}

#[no_mangle]
extern "C" fn libsettings_lock(ctx: *mut c_void) {
    let context: &Context = unsafe { &*(ctx as *const _) };
    context.lock.acquire();
}

#[no_mangle]
extern "C" fn libsettings_unlock(ctx: *mut c_void) {
    let context: &Context = unsafe { &*(ctx as *const _) };
    context.lock.release();
}

#[no_mangle]
extern "C" fn libsettings_wait_init(ctx: *mut c_void) -> i32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut _) };
    context.event = Some(Event::new());
    0
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::io::{Read, Write};

    use crossbeam_utils::thread::scope;
    use sbp::link::LinkSource;
    use sbp::messages::settings::{MsgSettingsReadReq, MsgSettingsReadResp};
    use sbp::{SbpIterExt, SbpString};

    static SETTINGS_SENDER_ID: u16 = 0x42;

    fn read_setting(
        rdr: impl Read + Send,
        mut wtr: impl Write + 'static,
        group: &str,
        name: &str,
    ) -> Option<Result<settings::SettingValue, Error<ReadSettingError>>> {
        scope(move |scope| {
            let source = LinkSource::new();
            let link = source.link();
            scope.spawn(move |_| {
                let messages = sbp::iter_messages(rdr).handle_errors(|e| panic!("{}", e));
                for message in messages {
                    source.send(message);
                }
            });
            let client = Client::new(link, move |msg| {
                sbp::to_writer(&mut wtr, &msg).map_err(Into::into)
            });
            client.read_setting(group, name)
        })
        .unwrap()
    }

    #[test]
    fn mock_read_setting_int() {
        let (group, name) = ("sbp", "obs_msg_max_size");
        let mut stream = mockstream::SyncMockStream::new();

        let request_msg = MsgSettingsReadReq {
            sender_id: Some(SETTINGS_SENDER_ID),
            setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
        };

        stream.wait_for(sbp::to_vec(&request_msg).unwrap().as_ref());

        let reply_msg = MsgSettingsReadResp {
            sender_id: Some(0x42),
            setting: SbpString::from(format!("{}\0{}\010\0", group, name).to_string()),
        };

        stream.push_bytes_to_read(sbp::to_vec(&reply_msg).unwrap().as_ref());

        let response = read_setting(stream.clone(), stream, group, name);

        assert!(matches!(response, Some(Ok(SettingValue::Integer(10)))));
    }

    #[test]
    fn mock_read_setting_bool() {
        let (group, name) = ("surveyed_position", "broadcast");
        let mut stream = mockstream::SyncMockStream::new();

        let request_msg = MsgSettingsReadReq {
            sender_id: Some(SETTINGS_SENDER_ID),
            setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
        };

        stream.wait_for(sbp::to_vec(&request_msg).unwrap().as_ref());

        let reply_msg = MsgSettingsReadResp {
            sender_id: Some(0x42),
            setting: SbpString::from(format!("{}\0{}\0True\0", group, name).to_string()),
        };

        stream.push_bytes_to_read(sbp::to_vec(&reply_msg).unwrap().as_ref());

        let response = read_setting(stream.clone(), stream, group, name);

        assert!(matches!(response, Some(Ok(SettingValue::Boolean(true)))));
    }

    #[test]
    fn mock_read_setting_double() {
        let (group, name) = ("surveyed_position", "surveyed_lat");
        let mut stream = mockstream::SyncMockStream::new();

        let request_msg = MsgSettingsReadReq {
            sender_id: Some(SETTINGS_SENDER_ID),
            setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
        };

        stream.wait_for(sbp::to_vec(&request_msg).unwrap().as_ref());

        let reply_msg = MsgSettingsReadResp {
            sender_id: Some(SETTINGS_SENDER_ID),
            setting: SbpString::from(format!("{}\0{}\00.1\0", group, name).to_string()),
        };

        stream.push_bytes_to_read(sbp::to_vec(&reply_msg).unwrap().as_ref());

        let response = read_setting(stream.clone(), stream, group, name)
            .unwrap()
            .unwrap();
        assert_eq!(response, SettingValue::Float(0.1));
    }

    #[test]
    fn mock_read_setting_string() {
        let (group, name) = ("rtcm_out", "ant_descriptor");
        let mut stream = mockstream::SyncMockStream::new();

        let request_msg = MsgSettingsReadReq {
            sender_id: Some(0x42),
            setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
        };

        stream.wait_for(sbp::to_vec(&request_msg).unwrap().as_ref());

        let reply_msg = MsgSettingsReadResp {
            sender_id: Some(SETTINGS_SENDER_ID),
            setting: SbpString::from(format!("{}\0{}\0foo\0", group, name).to_string()),
        };

        stream.push_bytes_to_read(sbp::to_vec(&reply_msg).unwrap().as_ref());

        let response = read_setting(stream.clone(), stream, group, name)
            .unwrap()
            .unwrap();
        assert_eq!(response, SettingValue::String("foo".into()));
    }

    #[test]
    fn mock_read_setting_enum() {
        let (group, name) = ("frontend", "antenna_selection");
        let mut stream = mockstream::SyncMockStream::new();

        let request_msg = MsgSettingsReadReq {
            sender_id: Some(SETTINGS_SENDER_ID),
            setting: SbpString::from(format!("{}\0{}\0", group, name).to_string()),
        };

        stream.wait_for(sbp::to_vec(&request_msg).unwrap().as_ref());

        let reply_msg = MsgSettingsReadResp {
            sender_id: Some(0x42),
            setting: SbpString::from(format!("{}\0{}\0Secondary\0", group, name).to_string()),
        };

        stream.push_bytes_to_read(sbp::to_vec(&reply_msg).unwrap().as_ref());

        let response = read_setting(stream.clone(), stream, group, name)
            .unwrap()
            .unwrap();
        assert_eq!(response, SettingValue::String("Secondary".into()));
    }
}
