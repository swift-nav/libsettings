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

#ifndef LIBSETTINGS_SETTING_TYPE_ENUM_H
#define LIBSETTINGS_SETTING_TYPE_ENUM_H

#include <stdbool.h>

#define LIBSETTINGS_ENUM_TAG "enum:"

#ifdef __cplusplus
extern "C" {
#endif

int enum_format_type(const void *priv, char *str, int slen);
int enum_to_string(const void *priv, char *str, int slen, const void *blob,
                   int blen);
bool enum_from_string(const void *priv, void *blob, int blen, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTING_TYPE_ENUM_H */
