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

#include <stddef.h>

#include <libsettings/settings_util.h>

/* Parse SBP message payload into setting parameters */
int settings_parse(const char *buf,
                   uint8_t blen,
                   const char **section,
                   const char **name,
                   const char **value,
                   const char **type)
{
  if (section) *section = NULL;
  if (name) *name = NULL;
  if (value) *value = NULL;
  if (type) *type = NULL;

  if (blen == 0) {
    return -1;
  }

  if (buf[blen - 1] != '\0') {
    return -1;
  }

  /* Extract parameters from message:
   * 3 null terminated strings: section, name and value
   * An optional fourth string is a description of the type.
   */
  int tok = 0;
  if (section) *section = (const char *)buf;
  for (int i = 0; i < blen; i++) {
    if (buf[i] != '\0') {
      continue;
    }
    tok++;
    switch (tok) {
    case 1:
      if (name == NULL) {
        continue;
      }
      *name = (const char *)&buf[i + 1];
      break;

    case 2:
      if (value == NULL) {
        continue;
      }
      if (i + 1 < blen) {
        *value = (const char *)&buf[i + 1];
      }
      break;

    case 3:
      if (type == NULL) {
        continue;
      }
      if (i + 1 < blen) {
        *type = (const char *)&buf[i + 1];
        break;
      }

    case 4:
      /* Enum list sentinel NULL */
      if (i == blen - 2) break;

    case 5:
      if (i == blen - 1) break;

    default: return -1;
    }
  }

  return tok;
}
