#!/usr/bin/env bash

bindgen \
    --allowlist-function settings_create \
    --allowlist-function settings_destroy \
    --allowlist-function settings_read_bool \
    --allowlist-function settings_read_by_idx \
    --allowlist-function settings_read_float \
    --allowlist-function settings_read_int \
    --allowlist-function settings_read_str \
    --allowlist-function settings_write_str \
    --allowlist-type settings_t \
    --allowlist-type sbp_msg_callback_t \
    --allowlist-type sbp_msg_callbacks_node_t \
    --allowlist-type settings_api_t \
    --allowlist-type settings_write_res_e_SETTINGS_WR_MODIFY_DISABLED \
    --allowlist-type settings_write_res_e_SETTINGS_WR_OK \
    --allowlist-type settings_write_res_e_SETTINGS_WR_PARSE_FAILED \
    --allowlist-type settings_write_res_e_SETTINGS_WR_READ_ONLY \
    --allowlist-type settings_write_res_e_SETTINGS_WR_SERVICE_FAILED \
    --allowlist-type settings_write_res_e_SETTINGS_WR_SETTING_REJECTED \
    --allowlist-type settings_write_res_e_SETTINGS_WR_TIMEOUT \
    --allowlist-type settings_write_res_e_SETTINGS_WR_VALUE_REJECTED \
    ./libsettings_wrapper.h \
    -o src/bindings.rs -- \
    -I../../include \
    -I../../third_party/libswiftnav/include \
    -I../../third_party/libsbp/c/include
