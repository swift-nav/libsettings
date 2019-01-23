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

#include <internal/setting_type_str.h>

static void check_to_string(const char *blob_str)
{
  char str[255] = {0};
  int res = str_to_string(NULL, str, sizeof(str), blob_str, 0);
  EXPECT_EQ(strlen(blob_str), res);
  EXPECT_STREQ(blob_str, str);
}

TEST(test_setting_type_str, to_string)
{
  check_to_string("test");
  check_to_string("");
}

static void check_from_string(const char *str)
{
  char blob_str[255] = {0};
  EXPECT_TRUE(str_from_string(NULL, blob_str, sizeof(blob_str), str));
  EXPECT_STREQ(str, blob_str);
}

TEST(test_setting_type_str, from_string)
{
  check_from_string("test");
  check_from_string("");
}
