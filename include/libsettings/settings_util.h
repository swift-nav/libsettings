/*
 * Copyright (C) 2018 Swift Navigation Inc.
 * Contact: Swift Navigation <dev@swiftnav.com>
 *
 * This source is subject to the license found in the file 'LICENSE' which must
 * be distributed together with this source. All other rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LIBSETTINGS_SETTINGS_UTIL_H
#define LIBSETTINGS_SETTINGS_UTIL_H

#include <inttypes.h>

typedef enum settings_tokens_e {
  SETTINGS_TOKENS_INVALID = -1,    /** An error occurred */
  SETTINGS_TOKENS_EMPTY = 0,       /** No tokens found */
  SETTINGS_TOKENS_SECTION = 1,     /** Section token found */
  SETTINGS_TOKENS_NAME = 2,        /** Section and name tokens found */
  SETTINGS_TOKENS_VALUE = 3,       /** Section, name and value tokens found */
  SETTINGS_TOKENS_TYPE = 4,        /** Section, name, value and type tokens found */
  SETTINGS_TOKENS_TYPE_CUSTOM = 5, /** Section, name, value and custom (enum) type tokens found */
} settings_tokens_t;

#ifdef __cplusplus
extern "C" {
#endif

int settings_format(const char *section,
                    const char *name,
                    const char *value,
                    const char *type,
                    char *buf,
                    uint8_t blen);

/**
 * @brief   Parse setting strings from SBP message buffer
 * @details Points the string pointers to corresponding message indexes.
 *          Function doesn't make copies from the strings so when the buffer
 *          is freed or out of scope the pointers shall be invalid. Each output
 *          pointer is optional and can be replaced with NULL.
 *
 * @param[in] buf           SBP message buffer
 * @param[in] blen          SBP message buffer length
 * @param[out] section      1st string, setting section
 * @param[out] name         2nd string, setting name
 * @param[out] value        3rd string, setting value
 * @param[out] type         4th string, setting type/enum list
 *
 * @return                  Number of NULL terminated tokens found. If more than
 *                          5 is found, -1 is returned. See @ settings_tokens_t.
 * @retval -1               An error occurred.
 */
settings_tokens_t settings_parse(const char *buf,
                                 uint8_t blen,
                                 const char **section,
                                 const char **name,
                                 const char **value,
                                 const char **type);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTINGS_UTIL_H */
