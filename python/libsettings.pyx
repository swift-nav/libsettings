# cython: language_level=2
# Copyright (C) 2018 Swift Navigation Inc.
# Contact: <dev@swift-nav.com>
#
# This source is subject to the license found in the file 'LICENSE' which must
# be be distributed together with this source. All other rights reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

cimport cython

from libc.stdint cimport uint8_t, uint16_t
from libc.stdint cimport uintptr_t

from sbp.msg import SBP, SENDER_ID

from threading import Event

cpdef enum SettingsWriteResponseCodes:
    SETTINGS_WR_OK = 0
    SETTINGS_WR_VALUE_REJECTED = 1
    SETTINGS_WR_SETTING_REJECTED = 2
    SETTINGS_WR_PARSE_FAILED = 3
    SETTINGS_WR_READ_ONLY = 4
    SETTINGS_WR_MODIFY_DISABLED = 5
    SETTINGS_WR_SERVICE_FAILED = 6
    SETTINGS_WR_TIMEOUT = 7

cdef extern from "../include/libsettings/settings.h":

    cdef struct settings_s:
        pass
    ctypedef settings_s settings_t

    cdef struct sbp_msg_callbacks_node:
        pass
    ctypedef sbp_msg_callbacks_node sbp_msg_callbacks_node_t

    ctypedef void (*sbp_msg_callback_t)(uint16_t sender_id, uint8_t len, uint8_t msg[], void *context)

    ctypedef int (*settings_send_t)(void *ctx, uint16_t msg_type, uint8_t len, uint8_t *payload)
    ctypedef int (*settings_send_from_t)(void *ctx,
                                         uint16_t msg_type,
                                         uint8_t len,
                                         uint8_t *payload,
                                         uint16_t sbp_sender_id)

    ctypedef int (*settings_wait_init_t)(void *ctx)
    ctypedef int (*settings_wait_t)(void *ctx, int timeout_ms)
    ctypedef int (*settings_wait_deinit_t)(void *ctx)
    ctypedef void (*settings_signal_t)(void *ctx)

    ctypedef int (*settings_reg_cb_t)(void *ctx,
                                      uint16_t msg_type,
                                      sbp_msg_callback_t cb,
                                      void *cb_context,
                                      sbp_msg_callbacks_node_t **node)
    ctypedef int (*settings_unreg_cb_t)(void *ctx, sbp_msg_callbacks_node_t **node)

    ctypedef void (*settings_log_t)(int priority, const char *format, ...)

    cdef struct settings_api_s:
        void *ctx
        settings_send_t send
        settings_send_from_t send_from
        settings_wait_init_t wait_init
        settings_wait_t wait
        settings_wait_deinit_t wait_deinit
        settings_signal_t signal
        settings_reg_cb_t register_cb
        settings_unreg_cb_t unregister_cb
        settings_log_t log
    ctypedef settings_api_s settings_api_t

    settings_t *settings_create(uint16_t sender_id, settings_api_t *api_impl)
    void settings_destroy(settings_t **ctx)

    ctypedef int settings_type_t
    ctypedef int (*settings_notify_fn)(void *ctx)

    int settings_register_setting(settings_t *ctx,
                                  const char *section,
                                  const char *name,
                                  void *var,
                                  size_t var_len,
                                  settings_type_t stype,
                                  settings_notify_fn notify,
                                  void *notify_context)

    int settings_write_str(settings_t *ctx,
                           const char *section,
                           const char *name,
                           const char *str)

    int settings_read_str(settings_t *ctx,
                          const char *section,
                          const char *name,
                          char *str,
                          size_t str_len)

    int settings_read_by_idx(settings_t *ctx,
                             uint16_t idx,
                             char *section,
                             size_t section_len,
                             char *name,
                             size_t name_len,
                             char *value,
                             size_t value_len,
                             char *fmt_type,
                             size_t type_len)


cdef class Settings:
    cdef settings_t *ctx
    cdef settings_api_t c_api

    cdef readonly object _callbacks
    cdef public object _event
    cdef readonly object _link
    cdef readonly object _debug

    def __init__(self, link, sender_id=SENDER_ID, debug=False):
        self.c_api.ctx = <void *>self
        self.c_api.send = &send_wrapper
        self.c_api.send_from = &send_from_wrapper
        self.c_api.wait = &wait_wrapper
        self.c_api.signal = &signal_wrapper
        self.c_api.register_cb = &register_cb_wrapper
        self.c_api.unregister_cb = &unregister_cb_wrapper
        self.c_api.log = &log_wrapper

        self.ctx = settings_create(sender_id, &self.c_api)

        self._link = link

        self._debug = debug

        self._callbacks = {}

    def destroy(self):
        settings_destroy(&self.ctx)

    def write(self, section, name, value):
        return settings_write_str(self.ctx,
                                  bytes(section),
                                  bytes(name),
                                  bytes(str(value)))

    def read(self, section, name):
        cdef char value[256]
        ret = settings_read_str(self.ctx,
                                bytes(section),
                                bytes(name),
                                value,
                                sizeof(value))
        if (ret == 0):
            return str(value)
        else:
            return None

    def read_all(self):
        cdef char section[256]
        cdef char name[256]
        cdef char value[256]
        cdef char fmt_type[256]
        cdef uint16_t idx = 0

        settings = []

        while (True):
            ret = settings_read_by_idx(self.ctx,
                                       idx,
                                       section,
                                       sizeof(section),
                                       name,
                                       sizeof(name),
                                       value,
                                       sizeof(value),
                                       fmt_type,
                                       sizeof(fmt_type))

            if (ret > 0):
                break
            elif (ret < 0):
                return []

            setting = {
                           'section': str(section),
                           'name': str(name),
                           'value': str(value),
                           'fmt_type': str(fmt_type),
                      }

            settings.append(setting)
            idx += 1

        return settings

    def _callback_broker(self, sbp_msg, **metadata):
        if self._debug:
            print '_callback_broker', sbp_msg.msg_type
            print ":".join("{:02x}".format(ord(c)) for c in sbp_msg.payload)

        msg_type = sbp_msg.msg_type

        cb_data = None

        for k,v in self._callbacks.items():
            # Only one callback per msg type
            if v[0] == msg_type:
                cb_data = self._callbacks[k]
                break

        if cb_data is None:
            raise Exception("Callback not registered for message type {}".format(msg_type))

        cdef sbp_msg_callback_t cb = <sbp_msg_callback_t><uintptr_t>cb_data[1]
        cb(sbp_msg.sender, sbp_msg.length, bytes(sbp_msg.payload), <void*><uintptr_t>cb_data[2])

cdef int send_wrapper(void *ctx, uint16_t msg_type, uint8_t length, uint8_t *payload):
    settings = <object>ctx
    
    if settings._debug:
        print 'send_wrapper', msg_type
        print ":".join("{:02x}".format(ord(c)) for c in payload[:length])

    # https://cython.readthedocs.io/en/latest/src/tutorial/strings.html
    # See section "Passing byte strings"
    settings._link(SBP(msg_type=msg_type,
                       length=length,
                       payload=payload[:length])) # Performs a copy of the data
    return 0

cdef int send_from_wrapper(void *ctx, uint16_t msg_type, uint8_t length, uint8_t *payload, uint16_t sbp_sender_id):
    settings = <object>ctx

    if settings._debug:
        print 'send_from_wrapper', msg_type
        print ":".join("{:02x}".format(ord(c)) for c in payload[:length])

    settings._link(SBP(msg_type=msg_type,
                       sender=sbp_sender_id,
                       length=length,
                       payload=payload[:length]))
    return 0

cdef int wait_wrapper(void *ctx, int timeout_ms):
    settings = <object>ctx
    settings._event = Event()
    res = settings._event.wait(timeout_ms / 1000.0)

    if res:
        return 0
    else:
        return -1

cdef void signal_wrapper(void *ctx):
    settings = <object>ctx
    settings._event.set()

cdef int register_cb_wrapper(void *ctx,
                             uint16_t msg_type,
                             sbp_msg_callback_t cb,
                             void *cb_context,
                             sbp_msg_callbacks_node_t **node):
    settings = <object>ctx
    settings._callbacks[<uintptr_t>node] = (msg_type, <uintptr_t>cb, <uintptr_t>cb_context)
    settings._link.add_callback(settings._callback_broker, msg_type)

    return 0

cdef int unregister_cb_wrapper(void *ctx, sbp_msg_callbacks_node_t **node):
    settings = <object>ctx
    cb_data = settings._callbacks[<uintptr_t>node]
    settings._link.remove_callback(settings._callback_broker, cb_data[0])
    del settings._callbacks[<uintptr_t>node]

    return 0

cdef void log_wrapper(int priority, const char *fmt, ...):
    # Currently no proper way to cythonize the variadic arguments..
    # https://github.com/cython/cython/wiki/FAQ#how-do-i-use-variable-args
    print fmt
