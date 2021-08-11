#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/libsettings.rs"));

use crate::settings_manager::{lookup_setting_type, SettingType};

use std::slice;

use std::io::{Read, Write};

use std::ffi::{CStr, CString};
use std::os::raw::c_char;

use std::boxed::Box;
use std::convert::TryInto;
use std::thread;

use std::mem;
use std::ptr;
use std::ptr::NonNull;

use std::time::Duration;

extern crate libc;
use libc::c_void;

use log::{debug, error, info, trace};

const SBP_STATE: sbp_state_t = sbp_state_t {
    state: 0,
    msg_type: 0,
    sender_id: 0,
    crc: 0,
    msg_len: 0,
    n_read: 0,
    msg_buff: ptr::null_mut(),
    io_context: ptr::null_mut(),
    sbp_msg_callbacks_head: ptr::null_mut() as *mut sbp_msg_callbacks_node_t,
    frame_buff: [0; 263],
    frame_len: 0,
};

static SENDER_ID: u16 = 0;

pub struct Context {
    libsettings_ctx: libsettings_ctx_t,
    sbp_state: sbp_state_t,
    stream_r: Box<dyn Read>,
    stream_w: Box<dyn Write>,
}

/* This wrapper allows us to pass a pointer to a Context object to a thread
 * so that an unsafe/FFI function can use the value.  The NonNull struct
 * implements Send for this purpose.
 */
struct ContextWrapper(NonNull<Context>);
unsafe impl Send for ContextWrapper {}

/* Allow libsettings_ctx_t to move across threads */
unsafe impl Send for libsettings_ctx_t {}

/* Allow sbp_state_t to move across threads */
unsafe impl Send for sbp_state_t {}

/* Allow Context to move across threads */
unsafe impl Send for Context {}

#[no_mangle]
pub extern "C" fn r_write(buff: *mut _u8, n: _u32, ctx: *mut libc::c_void) -> _s32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let slice = unsafe { slice::from_raw_parts(buff, n as usize) };

    match context.stream_w.write_all(slice) {
        Ok(()) => {
            return n as _s32;
        }
        Err(error) => {
            error!("r_write: error: {}!", error);
            return -1;
        }
    }
}

#[no_mangle]
pub extern "C" fn r_send(ctx: *mut c_void, msg_type: u16, len: u8, payload: *mut u8) -> i32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    unsafe {
        return sbp_send_message(
            &mut context.sbp_state,
            msg_type,
            SENDER_ID,
            len,
            payload,
            Some(r_write),
        ) as i32;
    }
}

#[no_mangle]
pub extern "C" fn r_send_from(
    ctx: *mut ::std::os::raw::c_void,
    msg_type: u16,
    len: u8,
    payload: *mut u8,
    sender_id: u16,
) -> i32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    unsafe {
        return sbp_send_message(
            &mut context.sbp_state,
            msg_type,
            sender_id,
            len,
            payload,
            Some(r_write),
        ) as i32;
    }
}

#[no_mangle]
pub extern "C" fn r_register_cb(
    ctx: *mut c_void,
    msg_type: u16,
    cb: sbp_msg_callback_t,
    cb_context: *mut c_void,
    node: *mut *mut sbp_msg_callbacks_node_t,
) -> i32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    unsafe {
        let mut v = vec![0u8; mem::size_of::<sbp_msg_callbacks_node_t>()];
        let n: *mut sbp_msg_callbacks_node_t = v.as_mut_ptr() as *mut sbp_msg_callbacks_node_t;
        let res = sbp_register_callback(&mut context.sbp_state, msg_type, cb, cb_context, n);
        if (node as i32) != 0 {
            *node = n;
            mem::forget(v);
        }
        return res as i32;
    }
}

#[no_mangle]
pub extern "C" fn r_unregister_cb(
    ctx: *mut c_void,
    node: *mut *mut sbp_msg_callbacks_node_t,
) -> i32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let res: _s8;
    unsafe {
        if (node as i32) != 0 {
            res = sbp_remove_callback(&mut context.sbp_state, *node);
            mem::drop(Vec::from_raw_parts(
                *node,
                0,
                mem::size_of::<sbp_msg_callbacks_node_t>(),
            ))
        } else {
            res = -127;
        }
    }
    return res as i32;
}

#[no_mangle]
pub extern "C" fn r_wait(ctx: *mut c_void, timeout_ms: i32) -> i32 {
    assert!(timeout_ms > 0);

    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let libsettings_ctx_ptr: *mut libsettings_ctx_t = &mut context.libsettings_ctx;

    let result: bool = unsafe { c_libsettings_wait(libsettings_ctx_ptr, timeout_ms as u32) };
    if !result {
        panic!("c_libsettings_wait failed");
    }

    return 0;
}

#[no_mangle]
pub extern "C" fn r_read(buff: *mut _u8, n: _u32, ctx: *mut c_void) -> _s32 {
    trace!("r_read: enter ({})!", n);

    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let read_slice = unsafe { slice::from_raw_parts_mut(buff, n as usize) };

    if let Ok(count) = context.stream_r.read(read_slice) {
        if count == 0 {
            error!("Connection closed");
            return -1;
        }
        debug!("r_read: OK ({})!\n", count);
        return count as _s32;
    }

    return -1;
}

#[no_mangle]
pub extern "C" fn r_lock(ctx: *mut c_void) {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let libsettings_ctx_ptr: *mut libsettings_ctx_t = &mut context.libsettings_ctx;

    let result: bool;
    unsafe { result = c_libsettings_lock(libsettings_ctx_ptr) }

    if !result {
        panic!("failed to acquire libsettings lock");
    }
}

#[no_mangle]
pub extern "C" fn r_unlock(ctx: *mut c_void) {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let libsettings_ctx_ptr: *mut libsettings_ctx_t = &mut context.libsettings_ctx;

    let result: bool = unsafe { c_libsettings_unlock(libsettings_ctx_ptr) };
    if !result {
        panic!("failed to release libsettings lock");
    }
}

#[no_mangle]
pub extern "C" fn r_signal(ctx: *mut c_void) {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let libsettings_ctx_ptr: *mut libsettings_ctx_t = &mut context.libsettings_ctx;
    let result: bool = unsafe { c_libsettings_signal(libsettings_ctx_ptr) };
    if !result {
        panic!("c_libsettings_signal failed");
    }
}

#[no_mangle]
pub extern "C" fn r_wait_init(ctx: *mut c_void) -> i32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let libsettings_ctx_ptr: *mut libsettings_ctx_t = &mut context.libsettings_ctx;
    let result: bool = unsafe { c_libsettings_lock(libsettings_ctx_ptr) };
    if result {
        return 0;
    } else {
        return -1;
    }
}

#[no_mangle]
pub extern "C" fn r_wait_deinit(ctx: *mut c_void) -> i32 {
    let context: &mut Context = unsafe { &mut *(ctx as *mut Context) };
    let libsettings_ctx_ptr: *mut libsettings_ctx_t = &mut context.libsettings_ctx;
    let result: bool = unsafe { c_libsettings_unlock(libsettings_ctx_ptr) };
    if result {
        return 0;
    } else {
        return -1;
    }
}

fn sbp_receive_thread(ctx: *mut Context) {
    debug!("Receive thread starting...");

    loop {
        let result: _s8 = unsafe { sbp_process(&mut (*ctx).sbp_state, Some(r_read)) };
        if result < SBP_OK as _s8 {
            break;
        }
    }

    debug!("Receive thread exiting...");
}

pub fn write_setting(
    stream_r: Box<dyn Read>,
    stream_w: Box<dyn Write>,
    section: String,
    name: String,
    value: String,
) {
    let mut context = Context {
        libsettings_ctx: libsettings_ctx_t {
            lock: ptr::null_mut(),
            condvar: ptr::null_mut(),
        },
        sbp_state: SBP_STATE,
        stream_r,
        stream_w,
    };

    let mut api: settings_api_t = settings_api_t {
        ctx: &mut context as *mut Context as *mut c_void,
        send: Some(r_send),
        send_from: Some(r_send_from),
        lock: Some(r_lock),
        unlock: Some(r_unlock),
        log: None,
        log_preformat: false,
        register_cb: Some(r_register_cb),
        unregister_cb: Some(r_unregister_cb),
        signal: Some(r_signal),
        signal_thd: None,
        wait: Some(r_wait),
        wait_init: Some(r_wait_init),
        wait_deinit: Some(r_wait_deinit),
        wait_thd: None,
    };

    let c_libsettings_init_result: bool =
        unsafe { c_libsettings_init(&mut context.libsettings_ctx) };

    if !c_libsettings_init_result {
        panic!("Failed to initialize libsettings binding library");
    }

    unsafe { sbp_state_init(&mut context.sbp_state) };
    unsafe {
        sbp_state_set_io_context(
            &mut context.sbp_state,
            &mut context as *mut Context as *mut c_void,
        )
    };

    let mut settings_ctx = unsafe { settings_create(SENDER_ID, &mut api) };
    let context_ptr = ContextWrapper(NonNull::new(&mut context as *mut Context).unwrap());

    let read_thread = thread::spawn(move || {
        sbp_receive_thread(context_ptr.0.as_ptr());
    });

    thread::sleep(Duration::from_millis(50));

    info!(
        "Writing setting: section={}, name={}, value={}",
        section.clone(),
        name.clone(),
        value
    );

    if let Some(type_) = lookup_setting_type(&section.clone(), &name.clone()) {
        let section = CString::new(section.clone()).unwrap();
        let name = CString::new(name.clone()).unwrap();
        let res = match type_ {
            SettingType::StInteger => {
                let value: i32 = value
                    .parse::<i32>()
                    .expect("failed to parse argument value");
                unsafe {
                    settings_write_int(
                        settings_ctx,
                        ptr::null_mut(),
                        section.as_ptr(),
                        name.as_ptr(),
                        value,
                    )
                }
            }
            SettingType::StBoolean => {
                let value: bool = value
                    .parse::<bool>()
                    .expect("failed to parse argument value");
                unsafe {
                    settings_write_bool(
                        settings_ctx,
                        ptr::null_mut(),
                        section.as_ptr(),
                        name.as_ptr(),
                        value,
                    )
                }
            }
            SettingType::StString => {
                let value_cstring = CString::new(value).unwrap();
                unsafe {
                    settings_write_str(
                        settings_ctx,
                        ptr::null_mut(),
                        section.as_ptr(),
                        name.as_ptr(),
                        value_cstring.as_ptr(),
                    )
                }
            }
            _ => 0, // settings_write_res_e_SETTINGS_WR_SERVICE_FAILED.try_into().unwrap(),  // todo
        };
        info!("Settings write result: {}", res);
    } else {
        error!("Unknown settings specified...");
    }

    read_thread.join().expect("failed to wait for read thread");

    unsafe {
        settings_destroy(&mut settings_ctx as *mut _);
    }
}

#[derive(Debug, PartialEq)]
pub enum SettingValue {
    integer(i32),
    boolean(bool),
    double(f32),
    string(Box<String>),
}

pub fn create_api(stream_r: Box<dyn Read>, stream_w: Box<dyn Write>) -> (Context, settings_api_t) {
    let mut context = Context {
        libsettings_ctx: libsettings_ctx_t {
            lock: ptr::null_mut(),
            condvar: ptr::null_mut(),
        },
        sbp_state: SBP_STATE,
        stream_r,
        stream_w,
    };

    let mut api = settings_api_t {
        ctx: &mut context as *mut Context as *mut c_void,
        send: Some(r_send),
        send_from: Some(r_send_from),
        lock: Some(r_lock),
        unlock: Some(r_unlock),
        log: None,
        log_preformat: false,
        register_cb: Some(r_register_cb),
        unregister_cb: Some(r_unregister_cb),
        signal: Some(r_signal),
        signal_thd: None,
        wait: Some(r_wait),
        wait_init: Some(r_wait_init),
        wait_deinit: Some(r_wait_deinit),
        wait_thd: None,
    };
    (context, api)
}

pub fn read_setting(
    //mut context: Context,
    //mut api: settings_api_t,
    stream_r: Box<dyn Read>,
    stream_w: Box<dyn Write>,
    section: String,
    name: String,
) -> SettingValue {
    let mut context = Context {
        libsettings_ctx: libsettings_ctx_t {
            lock: ptr::null_mut(),
            condvar: ptr::null_mut(),
        },
        sbp_state: SBP_STATE,
        stream_r,
        stream_w,
    };

    let mut api = settings_api_t {
        ctx: &mut context as *mut Context as *mut c_void,
        send: Some(r_send),
        send_from: Some(r_send_from),
        lock: Some(r_lock),
        unlock: Some(r_unlock),
        log: None,
        log_preformat: false,
        register_cb: Some(r_register_cb),
        unregister_cb: Some(r_unregister_cb),
        signal: Some(r_signal),
        signal_thd: None,
        wait: Some(r_wait),
        wait_init: Some(r_wait_init),
        wait_deinit: Some(r_wait_deinit),
        wait_thd: None,
    };

    let c_libsettings_init_result: bool =
        unsafe { c_libsettings_init(&mut context.libsettings_ctx) };

    if !c_libsettings_init_result {
        panic!("Failed to initialize libsettings binding library");
    }

    unsafe { sbp_state_init(&mut context.sbp_state) };
    unsafe {
        sbp_state_set_io_context(
            &mut context.sbp_state,
            &mut context as *mut Context as *mut c_void,
        )
    };

    let mut settings_ctx = unsafe { settings_create(SENDER_ID, &mut api) };
    let context_ptr = ContextWrapper(NonNull::new(&mut context as *mut Context).unwrap());

    let read_thread = thread::spawn(move || {
        sbp_receive_thread(context_ptr.0.as_ptr());
    });

    thread::sleep(Duration::from_millis(50));

    debug!(
        "Reading setting: section={}, name={}",
        section.clone(),
        name.clone(),
    );

    let c_section = CString::new(section.clone()).unwrap();
    let c_name = CString::new(name.clone()).unwrap();
    let mut return_value: SettingValue = SettingValue::integer(0);

    if let Some(type_) = lookup_setting_type(&section.clone(), &name.clone()) {
        let res = match type_ {
            SettingType::StInteger => unsafe {
                let mut value: i32 = 0;
                let status = settings_read_int(
                    settings_ctx,
                    c_section.as_ptr(),
                    c_name.as_ptr(),
                    &mut value,
                );
                return_value = SettingValue::integer(value);
                status
            },
            SettingType::StBoolean => unsafe {
                let mut value: bool = false;
                let status = settings_read_bool(
                    settings_ctx,
                    c_section.as_ptr(),
                    c_name.as_ptr(),
                    &mut value,
                );
                return_value = SettingValue::boolean(value);
                status
            },
            SettingType::StDouble => unsafe {
                let mut value: f32 = 0.0;
                let status = settings_read_float(
                    settings_ctx,
                    c_section.as_ptr(),
                    c_name.as_ptr(),
                    &mut value,
                );
                return_value = SettingValue::double(value);
                status
            },
            SettingType::StString | SettingType::StEnum => unsafe {
                let mut buf = Vec::<c_char>::with_capacity(1024);
                let status = settings_read_str(
                    settings_ctx,
                    c_section.as_ptr(),
                    c_name.as_ptr(),
                    buf.as_mut_ptr(),
                    buf.capacity().try_into().unwrap(),
                );
                return_value = SettingValue::string(Box::new(
                    CStr::from_ptr(buf.as_ptr().try_into().unwrap())
                        .to_string_lossy()
                        .into_owned(),
                ));
                status
            },
        };
        debug!("Settings read result: {}", res);
    } else {
        error!("Unknown settings specified...");
    }

    read_thread.join().expect("failed to wait for read thread");

    unsafe {
        settings_destroy(&mut settings_ctx as *mut _);
    }

    return_value
}
