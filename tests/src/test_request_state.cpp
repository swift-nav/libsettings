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

#include "gtest/gtest.h"

#include <libsettings/settings.h>

#include <internal/request_state.h>

#include <test_stubs.hh>

TEST(test_request_state, init_deinit) {
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

TEST(test_request_state, match) {
  request_state_t state = {0};

  EXPECT_EQ(false, request_state_match(&state));
}

TEST(test_request_state, check) {
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

  EXPECT_EQ(NULL, request_state_check(settings, test_data, strlen(test_data)));

  request_state_append(settings, &state);

  EXPECT_EQ(&state,
            request_state_check(settings, test_data, strlen(test_data)));

  request_state_remove(settings, &state);

  EXPECT_EQ(NULL, request_state_check(settings, test_data, strlen(test_data)));

  state.pending = false;

  EXPECT_EQ(NULL, request_state_check(settings, test_data, strlen(test_data)));
}
