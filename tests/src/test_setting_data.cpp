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

#include <internal/setting_data.h>
#include <internal/setting_type_enum.h>

TEST(test_setting_data, format) {
  type_data_t *type_data_list = NULL;
  const char *const enum_names[] = {"Test1", "Test2", NULL};
  settings_type_t type;

  int res = type_register(&type_data_list,
                          enum_to_string,
                          enum_from_string,
                          enum_format_type,
                          enum_names,
                          &type);

  EXPECT_EQ(0, res);

  int var = 0;

  setting_data_t *setting_data = setting_data_create(type_data_list,
                                                     "section",
                                                     "name",
                                                     &var,
                                                     sizeof(var),
                                                     type,
                                                     NULL,
                                                     NULL,
                                                     true,
                                                     false);

  char buf[255] = {0};
  res = setting_data_format(setting_data, true, buf, sizeof(buf), NULL);

  char expected[] = {'s', 'e', 'c', 't', 'i', 'o', 'n', '\0',
                     'n', 'a', 'm', 'e', '\0',
                     'T', 'e', 's', 't', '1', '\0',
                     'e', 'n', 'u', 'm', ':', 'T', 'e', 's', 't', '1', ',', 'T', 'e', 's', 't', '2', '\0'};

  EXPECT_EQ(sizeof(expected), res);

  for (int i = 0; i < sizeof(expected); ++i) {
    EXPECT_EQ(expected[i], buf[i]);
  }
}
