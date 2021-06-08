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

#include <limits>

#include <internal/setting_type_int.h>

TEST(test_setting_type_int, to_string) {
  char str[255] = {0};

  int8_t blob8 = std::numeric_limits<int8_t>::min();
  int res = int_to_string(NULL, str, sizeof(str), &blob8, sizeof(blob8));
  EXPECT_EQ(std::to_string(blob8).length(), res);
  EXPECT_STREQ(std::to_string(blob8).c_str(), str);

  blob8 = std::numeric_limits<int8_t>::max();
  res = int_to_string(NULL, str, sizeof(str), &blob8, sizeof(blob8));
  EXPECT_EQ(std::to_string(blob8).length(), res);
  EXPECT_STREQ(std::to_string(blob8).c_str(), str);

  int16_t blob16 = std::numeric_limits<int16_t>::min();
  res = int_to_string(NULL, str, sizeof(str), &blob16, sizeof(blob16));
  EXPECT_EQ(std::to_string(blob16).length(), res);
  EXPECT_STREQ(std::to_string(blob16).c_str(), str);

  blob16 = std::numeric_limits<int16_t>::max();
  res = int_to_string(NULL, str, sizeof(str), &blob16, sizeof(blob16));
  EXPECT_EQ(std::to_string(blob16).length(), res);
  EXPECT_STREQ(std::to_string(blob16).c_str(), str);

  int32_t blob32 = std::numeric_limits<int32_t>::min();
  res = int_to_string(NULL, str, sizeof(str), &blob32, sizeof(blob32));
  EXPECT_EQ(std::to_string(blob32).length(), res);
  EXPECT_STREQ(std::to_string(blob32).c_str(), str);

  blob32 = std::numeric_limits<int32_t>::max();
  res = int_to_string(NULL, str, sizeof(str), &blob32, sizeof(blob32));
  EXPECT_EQ(std::to_string(blob32).length(), res);
  EXPECT_STREQ(std::to_string(blob32).c_str(), str);
}

TEST(test_setting_type_int, from_string) {
  const char *str = std::to_string(std::numeric_limits<int8_t>::min()).c_str();
  int8_t blob8 = 0;
  EXPECT_TRUE(int_from_string(NULL, &blob8, sizeof(blob8), str));
  EXPECT_EQ(std::numeric_limits<int8_t>::min(), blob8);

  str = std::to_string(std::numeric_limits<int8_t>::max()).c_str();
  EXPECT_TRUE(int_from_string(NULL, &blob8, sizeof(blob8), str));
  EXPECT_EQ(std::numeric_limits<int8_t>::max(), blob8);

  str = std::to_string(std::numeric_limits<int16_t>::min()).c_str();
  int16_t blob16 = 0;
  EXPECT_TRUE(int_from_string(NULL, &blob16, sizeof(blob16), str));
  EXPECT_EQ(std::numeric_limits<int16_t>::min(), blob16);

  str = std::to_string(std::numeric_limits<int16_t>::max()).c_str();
  EXPECT_TRUE(int_from_string(NULL, &blob16, sizeof(blob16), str));
  EXPECT_EQ(std::numeric_limits<int16_t>::max(), blob16);

  str = std::to_string(std::numeric_limits<int32_t>::min()).c_str();
  int32_t blob32 = 0;
  EXPECT_TRUE(int_from_string(NULL, &blob32, sizeof(blob32), str));
  EXPECT_EQ(std::numeric_limits<int32_t>::min(), blob32);

  str = std::to_string(std::numeric_limits<int32_t>::max()).c_str();
  EXPECT_TRUE(int_from_string(NULL, &blob32, sizeof(blob32), str));
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), blob32);
}
