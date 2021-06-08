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

#include <cmath>
#include <limits>
#include <sstream>

#include <internal/setting_type_float.h>

static double to_precision(double num, int precision) {
  std::ostringstream str;
  str << std::setprecision(precision);
  str << num;
  return std::stod(str.str(), NULL);
}

TEST(test_setting_type_float, to_string) {
  char str[255] = {0};

  float blob_float = pow(10, -1 * SETTINGS_FLOAT_PRECISION);
  int res =
      float_to_string(NULL, str, sizeof(str), &blob_float, sizeof(blob_float));
  EXPECT_FLOAT_EQ(blob_float, std::stod(str, NULL));

  blob_float = std::numeric_limits<float>::max();
  res =
      float_to_string(NULL, str, sizeof(str), &blob_float, sizeof(blob_float));
  EXPECT_FLOAT_EQ(blob_float, std::stod(str, NULL));

  double blob_double = pow(10, -1 * SETTINGS_FLOAT_PRECISION);
  res = float_to_string(NULL, str, sizeof(str), &blob_double,
                        sizeof(blob_double));
  EXPECT_DOUBLE_EQ(blob_double, std::stod(str, NULL));

  blob_double = std::numeric_limits<double>::max();
  res = float_to_string(NULL, str, sizeof(str), &blob_double,
                        sizeof(blob_double));
  EXPECT_DOUBLE_EQ(to_precision(blob_double, SETTINGS_FLOAT_PRECISION),
                   std::stod(str, NULL));
}

TEST(test_setting_type_float, from_string) {
  std::string str = "1e-" SETTINGS_FLOAT_PRECISION_STR;
  float blob_float = 0;
  EXPECT_TRUE(
      float_from_string(NULL, &blob_float, sizeof(blob_float), str.c_str()));
  EXPECT_FLOAT_EQ(1e-12, blob_float);

  str = std::to_string(std::numeric_limits<float>::max());
  EXPECT_TRUE(
      float_from_string(NULL, &blob_float, sizeof(blob_float), str.c_str()));
  EXPECT_FLOAT_EQ(to_precision(std::numeric_limits<float>::max(), 12),
                  blob_float);

  str = "1e-" SETTINGS_FLOAT_PRECISION_STR;
  double blob_double = 0;
  EXPECT_TRUE(
      float_from_string(NULL, &blob_double, sizeof(blob_double), str.c_str()));
  EXPECT_DOUBLE_EQ(pow(10, -1 * SETTINGS_FLOAT_PRECISION), blob_double);

  str = std::to_string(std::numeric_limits<double>::max());
  EXPECT_TRUE(
      float_from_string(NULL, &blob_double, sizeof(blob_double), str.c_str()));
  EXPECT_DOUBLE_EQ(std::numeric_limits<double>::max(), blob_double);
}
