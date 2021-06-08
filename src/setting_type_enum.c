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
#include <string.h>

#include <internal/setting_type_enum.h>

int enum_format_type(const void *priv, char *str, int slen) {
  int n = 0;
  int l;

  /* Print "enum:" header */
  l = snprintf(&str[n], slen - n, LIBSETTINGS_ENUM_TAG);
  if (l < 0) {
    return l;
  }
  n += l;

  /* Print enum names separated by commas */
  for (const char *const *enum_names = priv; *enum_names; enum_names++) {
    if (n < slen) {
      l = snprintf(&str[n], slen - n, "%s,", *enum_names);
      if (l < 0) {
        return l;
      }
      n += l;
    } else {
      n += strlen(*enum_names) + 1;
    }
  }

  /* Replace last comma with NULL */
  if ((n > (int)strlen(LIBSETTINGS_ENUM_TAG)) && (n - 1 < slen)) {
    str[n - 1] = '\0';
    n--;
  }

  return n;
}

int enum_to_string(const void *priv, char *str, int slen, const void *blob,
                   int blen) {
  (void)blen;

  const char *const *enum_names = priv;
  int index = *(uint8_t *)blob;
  return snprintf(str, slen, "%s", enum_names[index]);
}

bool enum_from_string(const void *priv, void *blob, int blen, const char *str) {
  (void)blen;

  const char *const *enum_names = priv;
  int i;

  for (i = 0; enum_names[i] && (strcmp(str, enum_names[i]) != 0); i++) {
    ;
  }

  if (!enum_names[i]) {
    return false;
  }

  *(uint8_t *)blob = i;

  return true;
}
