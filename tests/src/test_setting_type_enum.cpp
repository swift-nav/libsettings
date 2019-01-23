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

#include <internal/setting_type_enum.h>

static void test_format(const char *const enum_names[], std::string expected)
{
  char buf[255] = {0};

  int len = enum_format_type(enum_names, buf, sizeof(buf));

  EXPECT_EQ(expected.length(), len);
  EXPECT_EQ(expected, std::string(buf));
}

TEST(test_setting_type_enum, format)
{
  const char *const bool_enum_names[] = {"False", "True", NULL};
  test_format(bool_enum_names, std::string(LIBSETTINGS_ENUM_TAG "False,True"));

  const char *const empty_enum_names[] = {NULL};
  test_format(empty_enum_names, std::string(LIBSETTINGS_ENUM_TAG));
}
