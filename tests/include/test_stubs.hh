/*
 * Copyright (C) 2019 Swift Navigation Inc.
 * Contact: Swift Navigation <dev@swiftnav.com>
 *
 * This source is subject to the license found in the file 'LICENSE' which must
 * be distributed together with this source. All other rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <libsettings/settings.h>

#include <internal/request_state.h>

void log_dummy(int priority, const char *format, ...);

int send_dummy(void *ctx, uint16_t msg_type, uint8_t len, uint8_t *payload);

int send_from_dummy(void *ctx,
                           uint16_t msg_type,
                           uint8_t len,
                           uint8_t *payload,
                           uint16_t sbp_sender_id);

int wait_init_dummy(void *ctx);

int wait_dummy(void *ctx, int timeout_ms);

int wait_deinit_dummy(void *ctx);

void signal_dummy(void *ctx);

int wait_thd_dummy(void *ctx, int timeout_ms);

void signal_thd_dummy(void *ctx);

void lock_dummy(void *ctx);

void unlock_dummy(void *ctx);

int reg_cb_dummy(void *ctx,
                        uint16_t msg_type,
                        sbp_msg_callback_t cb,
                        void *cb_context,
                        sbp_msg_callbacks_node_t **node);

int unreg_cb_dummy(void *ctx, sbp_msg_callbacks_node_t **node);

