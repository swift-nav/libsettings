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

#include <stdio.h>
#include <stdarg.h>

#include "gtest/gtest.h"

#include <libsettings/settings.h>

#include <internal/request_state.h>

static void log_dummy(int priority, const char *format, ...)
{
  va_list args;
  va_start(args, format);

  vprintf(format, args);

  va_end(args);
}

static int send_dummy(void *ctx, uint16_t msg_type, uint8_t len, uint8_t *payload)
{
  (void)ctx;
  (void)msg_type;
  (void)len;
  (void)payload;
  return 0;
}

static int send_from_dummy(void *ctx,
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

static int wait_init_dummy(void *ctx)
{
  (void)ctx;
  return 0;
}

static int wait_dummy(void *ctx, int timeout_ms)
{
  (void)ctx;
  (void)timeout_ms;
  return 0;
}

static int wait_deinit_dummy(void *ctx)
{
  (void)ctx;
  return 0;
}

static void signal_dummy(void *ctx)
{
  (void)ctx;
}

static int wait_thd_dummy(void *ctx, int timeout_ms)
{
  (void)ctx;
  (void)timeout_ms;
  return 0;
}

static void signal_thd_dummy(void *ctx)
{
  (void)ctx;
}

static void lock_dummy(void *ctx)
{
  (void)ctx;
}

static void unlock_dummy(void *ctx)
{
  (void)ctx;
}

static int reg_cb_dummy(void *ctx,
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

static int unreg_cb_dummy(void *ctx, sbp_msg_callbacks_node_t **node)
{
  (void)ctx;
  (void)node;
  return 0;
}

TEST(test_request_state, init_deinit)
{
  request_state_t state = {0};
  std::string test_str = "testing";
  const char *test_data = test_str.c_str();

  request_state_init(&state, NULL, 7, test_data, strlen(test_data));

  EXPECT_EQ(7, state.msg_id);
  EXPECT_EQ(false, state.match);
  EXPECT_EQ(true, state.pending);
  EXPECT_EQ(strlen(test_data), state.compare_data_len);
  EXPECT_STREQ(test_data, (char *)state.compare_data);

  request_state_deinit(&state);

  EXPECT_EQ(false, state.pending);
}

TEST(test_request_state, match)
{
  request_state_t state = {0};

  EXPECT_EQ(false, request_state_match(&state));
}

TEST(test_request_state, check)
{
  static settings_t *settings = NULL;

  settings_api_t api = {
    api.ctx = NULL,
    api.send = send_dummy,
    api.send_from = send_from_dummy,
    api.wait_init = wait_init_dummy,
    api.wait = wait_dummy,
    api.wait_deinit = wait_deinit_dummy,
    api.signal = signal_dummy,
    api.wait = wait_thd_dummy,
    api.signal = signal_thd_dummy,
    api.lock = lock_dummy,
    api.unlock = unlock_dummy,
    api.register_cb = reg_cb_dummy,
    api.unregister_cb = unreg_cb_dummy,
    api.log = log_dummy,
  };

  settings = settings_create(0x42, &api);

  request_state_t state = {0};
  std::string test_str = "testing";
  const char *test_data = test_str.c_str();

  EXPECT_DEATH(request_state_check(NULL, NULL, 0), "");

  request_state_init(&state, NULL, 9, test_data, strlen(test_data));

  EXPECT_EQ(NULL, request_state_check(settings, test_data, strlen(test_data) - 1));

  // EXPECT_DEATH(request_state_check(settings, test_data, strlen(test_data)), "");

  state.pending = false;

  EXPECT_EQ(NULL, request_state_check(settings, test_data, strlen(test_data)));
}
