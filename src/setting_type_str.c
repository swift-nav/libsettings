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

#include <inttypes.h>
#include <stdio.h>

#include <internal/setting_type_str.h>

int str_to_string(const void *priv, char *str, int slen, const void *blob,
                  int blen) {
  (void)priv;
  (void)blen;

  return snprintf(str, slen, "%s", (char *)blob);
}

bool str_from_string(const void *priv, void *blob, int blen, const char *str) {
  (void)priv;

  int l = snprintf(blob, blen, "%s", str);
  if ((l < 0) || (l >= blen)) {
    return false;
  }

  return true;
}
