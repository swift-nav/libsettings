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

#include <libsettings/settings_util.h>

TEST(test_settings_util, parse)
{
  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;

  char msg[1] = {1};
  EXPECT_EQ(SETTINGS_TOKENS_INVALID,
            settings_parse(msg, sizeof(msg), &section, &name, &value, &type));
  EXPECT_EQ(SETTINGS_TOKENS_EMPTY, settings_parse(msg, 0, &section, &name, &value, &type));

  char msg1[1] = {'\0'};
  EXPECT_EQ(SETTINGS_TOKENS_SECTION,
            settings_parse(msg1, sizeof(msg1), &section, &name, &value, &type));

  char msg2[2] = {'\0', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_NAME,
            settings_parse(msg2, sizeof(msg2), &section, &name, &value, &type));

  char msg3[3] = {'\0', '\0', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_VALUE,
            settings_parse(msg3, sizeof(msg3), &section, &name, &value, &type));

  char msg4[4] = {'\0', '\0', '\0', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_TYPE,
            settings_parse(msg4, sizeof(msg4), &section, &name, &value, &type));
  EXPECT_EQ(SETTINGS_TOKENS_TYPE, settings_parse(msg4, sizeof(msg4), NULL, &name, &value, &type));
  EXPECT_EQ(SETTINGS_TOKENS_TYPE, settings_parse(msg4, sizeof(msg4), NULL, NULL, &value, &type));
  EXPECT_EQ(SETTINGS_TOKENS_TYPE, settings_parse(msg4, sizeof(msg4), NULL, NULL, NULL, &type));
  EXPECT_EQ(SETTINGS_TOKENS_TYPE, settings_parse(msg4, sizeof(msg4), NULL, NULL, NULL, NULL));

  char msg5[5] = {'\0', '\0', '\0', '\0', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_EXTRA_NULL,
            settings_parse(msg5, sizeof(msg5), &section, &name, &value, &type));

  char msg6[6] = {'\0', '\0', '\0', '\0', '\0', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_INVALID,
            settings_parse(msg6, sizeof(msg6), &section, &name, &value, &type));

  char sect[5] = {'s', 'e', 'c', 't', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_SECTION,
            settings_parse(sect, sizeof(sect), &section, &name, &value, &type));
  EXPECT_STREQ("sect", section);
  EXPECT_EQ(NULL, name);
  EXPECT_EQ(NULL, value);
  EXPECT_EQ(NULL, type);

  char sect_name[10] = {'s', 'e', 'c', 't', '\0', 'n', 'a', 'm', 'e', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_NAME,
            settings_parse(sect_name, sizeof(sect_name), &section, &name, &value, &type));
  EXPECT_STREQ("sect", section);
  EXPECT_STREQ("name", name);
  EXPECT_EQ(NULL, value);
  EXPECT_EQ(NULL, type);

  char sect_name_val[16] =
    {'s', 'e', 'c', 't', '\0', 'n', 'a', 'm', 'e', '\0', 'v', 'a', 'l', 'u', 'e', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_VALUE,
            settings_parse(sect_name_val, sizeof(sect_name_val), &section, &name, &value, &type));
  EXPECT_STREQ("sect", section);
  EXPECT_STREQ("name", name);
  EXPECT_STREQ("value", value);
  EXPECT_EQ(NULL, type);

  char sect_name_val_type[21] = {'s', 'e', 'c', 't', '\0', 'n', 'a', 'm', 'e', '\0', 'v',
                                 'a', 'l', 'u', 'e', '\0', 't', 'y', 'p', 'e', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_TYPE,
            settings_parse(sect_name_val_type,
                           sizeof(sect_name_val_type),
                           &section,
                           &name,
                           &value,
                           &type));
  EXPECT_STREQ("sect", section);
  EXPECT_STREQ("name", name);
  EXPECT_STREQ("value", value);
  EXPECT_STREQ("type", type);

  /* Backwards compatibility */
  char sect_name_val_enumtype[27] = {'s',  'e', 'c', 't', '\0', 'n', 'a',  'm',  'e',
                                     '\0', 'v', 'a', 'l', 'u',  'e', '\0', 'e',  'n',
                                     'u',  'm', ',', 't', 'y',  'p', 'e',  '\0', '\0'};
  EXPECT_EQ(SETTINGS_TOKENS_EXTRA_NULL,
            settings_parse(sect_name_val_enumtype,
                           sizeof(sect_name_val_enumtype),
                           &section,
                           &name,
                           &value,
                           &type));
  EXPECT_STREQ("sect", section);
  EXPECT_STREQ("name", name);
  EXPECT_STREQ("value", value);
  EXPECT_STREQ("enum,type", type);

  char sect_name_val_type_noterm[] = {'s',  'e', 'c', 't', '\0', 'n', 'a',  'm', 'e',
                                      '\0', 'v', 'a', 'l', 'u',  'e', '\0', 'e', 'n',
                                      'u',  'm', ',', 't', 'y',  'p', 'e'};
  EXPECT_EQ(SETTINGS_TOKENS_INVALID,
            settings_parse(sect_name_val_type_noterm,
                           sizeof(sect_name_val_type_noterm),
                           &section,
                           &name,
                           &value,
                           &type));
  EXPECT_EQ(NULL, section);
  EXPECT_EQ(NULL, name);
  EXPECT_EQ(NULL, value);
  EXPECT_EQ(NULL, type);
}
