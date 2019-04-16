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

#include <stdarg.h>
#include <stdio.h>

#include <test_stubs.hh>

void log_dummy(int priority, const char *format, ...)
{
  va_list args;
  va_start(args, format);

  vprintf(format, args);

  va_end(args);
}

int send_dummy(void *ctx, uint16_t msg_type, uint8_t len, uint8_t *payload)
{
  (void)ctx;
  (void)msg_type;
  (void)len;
  (void)payload;
  return 0;
}

int send_from_dummy(void *ctx,
                    uint16_t msg_type,
                    uint8_t len,
                    uint8_t *payload,
                    uint16_t sbp_sender_id)
{
  (void)ctx;
  (void)msg_type;
  (void)len;
  (void)payload;
  (void)sbp_sender_id;
  return 0;
}

int wait_init_dummy(void *ctx)
{
  (void)ctx;
  return 0;
}

int wait_dummy(void *ctx, int timeout_ms)
{
  (void)ctx;
  (void)timeout_ms;
  return 0;
}

int wait_deinit_dummy(void *ctx)
{
  (void)ctx;
  return 0;
}

void signal_dummy(void *ctx)
{
  (void)ctx;
}

int wait_thd_dummy(void *ctx, int timeout_ms)
{
  (void)ctx;
  (void)timeout_ms;
  return 0;
}

void signal_thd_dummy(void *ctx)
{
  (void)ctx;
}

void lock_dummy(void *ctx)
{
  (void)ctx;
}

void unlock_dummy(void *ctx)
{
  (void)ctx;
}

int reg_cb_dummy(void *ctx,
                 uint16_t msg_type,
                 sbp_msg_callback_t cb,
                 void *cb_context,
                 sbp_msg_callbacks_node_t **node)
{
  (void)ctx;
  (void)msg_type;
  (void)cb;
  (void)cb_context;
  (void)node;
  return 0;
}

int unreg_cb_dummy(void *ctx, sbp_msg_callbacks_node_t **node)
{
  (void)ctx;
  (void)node;
  return 0;
}
