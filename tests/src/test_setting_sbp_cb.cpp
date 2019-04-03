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

#include <libsbp/settings.h>

#include <libsettings/settings.h>
#include <internal/setting_sbp_cb.h>

#include <test_stubs.hh>

static settings_t *settings = NULL;

void settings_api_setup(void)
{
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
}

TEST(test_setting_sbp_cb, registration)
{
  settings_api_setup();

  /* Empty list */
  ASSERT_EQ(1, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_REGISTER_RESP));

  /* Add/remove one */
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_REGISTER_RESP));
  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_REGISTER_RESP));

  /* Add/remove all */
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_REGISTER_RESP));
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_WRITE));
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_WRITE_RESP));
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_READ_RESP));
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_READ_BY_INDEX_RESP));
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_READ_BY_INDEX_DONE));

  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_REGISTER_RESP));
  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_WRITE));
  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_WRITE_RESP));
  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_READ_RESP));
  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_READ_BY_INDEX_RESP));
  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_READ_BY_INDEX_DONE));

  /* Once more from empty list */
  ASSERT_EQ(1, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_READ_BY_INDEX_DONE));

  /* Add twice */
  ASSERT_EQ(0, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_REGISTER_RESP));
  ASSERT_EQ(1, setting_sbp_cb_register(settings, SBP_MSG_SETTINGS_REGISTER_RESP));

  /* Try to remove unregistered from not empty list */
  ASSERT_EQ(1, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_READ_BY_INDEX_DONE));
  ASSERT_EQ(0, setting_sbp_cb_unregister(settings, SBP_MSG_SETTINGS_REGISTER_RESP));

  settings_destroy(&settings);
}
