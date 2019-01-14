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

#ifndef LIBSETTINGS_SETTING_TYPE_FLOAT_H
#define LIBSETTINGS_SETTING_TYPE_FLOAT_H

#include <stdbool.h>

#define SETTINGS_FLOAT_PRECISION      12
#define SETTINGS_FLOAT_PRECISION_STR "12"

#ifdef __cplusplus
extern "C" {
#endif

int float_to_string(const void *priv, char *str, int slen, const void *blob, int blen);
bool float_from_string(const void *priv, void *blob, int blen, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTING_TYPE_FLOAT_H */
