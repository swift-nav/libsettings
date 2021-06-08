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

static type_data_t *type_data_list = NULL;
static const char *const enum_names[] = {"Test1", "Test2", NULL};

static setting_data_t *create_setting_data() {
  settings_type_t type;

  int res = type_register(&type_data_list, enum_to_string, enum_from_string,
                          enum_format_type, enum_names, &type);

  EXPECT_EQ(0, res);

  static int var = 0;

  setting_data_t *setting_data =
      setting_data_create(type_data_list, "section", "name", &var, sizeof(var),
                          type, NULL, NULL, true, false);

  return setting_data;
}

TEST(test_setting_data, format) {
  setting_data_t *setting_data = create_setting_data();

  ASSERT_TRUE(setting_data != NULL);

  char buf[255] = {0};
  int res = setting_data_format(setting_data, true, buf, sizeof(buf), NULL);

  char expected[] = {'s',  'e', 'c', 't',  'i', 'o', 'n', '\0', 'n',
                     'a',  'm', 'e', '\0', 'T', 'e', 's', 't',  '1',
                     '\0', 'e', 'n', 'u',  'm', ':', 'T', 'e',  's',
                     't',  '1', ',', 'T',  'e', 's', 't', '2',  '\0'};

  EXPECT_EQ(sizeof(expected), res);

  for (int i = 0; i < sizeof(expected); ++i) {
    EXPECT_EQ(expected[i], buf[i]);
  }
}

TEST(test_setting_data, list) {
  setting_data_t *list = NULL;
  setting_data_t *setting_data1 = create_setting_data();
  setting_data_t *setting_data2 = create_setting_data();
  setting_data_t *setting_data3 = create_setting_data();

  setting_data_append(&list, setting_data1);
  setting_data_append(&list, setting_data2);
  setting_data_append(&list, setting_data3);

  // Middle
  setting_data_remove(&list, &setting_data2);
  ASSERT_TRUE(list != NULL);
  ASSERT_TRUE(setting_data2 == NULL);
  // End
  setting_data_remove(&list, &setting_data3);
  ASSERT_TRUE(list != NULL);
  ASSERT_TRUE(setting_data3 == NULL);
  // One
  setting_data_remove(&list, &setting_data1);
  ASSERT_TRUE(list == NULL);
  ASSERT_TRUE(setting_data1 == NULL);
}
