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

#include <internal/setting_type_int.h>

int int_to_string(const void *priv, char *str, int slen, const void *blob, int blen)
{
  (void)priv;

  switch (blen) {
  case 1: {
    int16_t tmp = *(int8_t *)blob;
    /* mingw's crappy snprintf doesn't understand %hhd */
    return snprintf(str, slen, "%hd", tmp);
  }
  case 2: return snprintf(str, slen, "%hd", *(int16_t *)blob);
  case 4: return snprintf(str, slen, "%" PRId32, *(int32_t *)blob);
  default: return -1;
  }
}

bool int_from_string(const void *priv, void *blob, int blen, const char *str)
{
  (void)priv;

  switch (blen) {
  case 1: {
    int16_t tmp;
    /* Newlib's crappy sscanf doesn't understand %hhd */
    if (sscanf(str, "%hd", &tmp) == 1) {
      *(int8_t *)blob = tmp;
      return true;
    }
    return false;
  }
  case 2: return sscanf(str, "%hd", (int16_t *)blob) == 1;
  case 4: return sscanf(str, "%" PRId32, (int32_t *)blob) == 1;
  default: return false;
  }
}
