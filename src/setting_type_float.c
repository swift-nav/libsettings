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

#include <internal/setting_type_float.h>

int float_to_string(const void *priv, char *str, int slen, const void *blob, int blen)
{
  (void)priv;

  switch (blen) {
  case 4: return snprintf(str, slen, "%."SETTINGS_FLOAT_PRECISION_STR"g", (double)*(float *)blob);
  case 8: return snprintf(str, slen, "%."SETTINGS_FLOAT_PRECISION_STR"g", *(double *)blob);
  default: return -1;
  }
}

bool float_from_string(const void *priv, void *blob, int blen, const char *str)
{
  (void)priv;

  switch (blen) {
  case 4: return sscanf(str, "%g", (float *)blob) == 1;
  case 8: return sscanf(str, "%lg", (double *)blob) == 1;
  default: return false;
  }
}
