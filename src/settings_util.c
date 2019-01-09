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
#include <stdio.h>

#include <libsettings/settings_util.h>

int settings_format(const char *section,
                    const char *name,
                    const char *value,
                    const char *type,
                    char *buf,
                    uint8_t blen)
{
  int n = 0;
  int l = 0;

  const char *tokens[SETTINGS_TOKENS_TYPE] = {section, name, value, type};

  for (int i = 0; i < SETTINGS_TOKENS_TYPE; ++i) {
    const char *token = tokens[i];

    if (token == NULL) {
      break;
    }

    l = snprintf(&buf[n], blen - n, "%s", token);

    if ((l < 0) || (l >= blen - n)) {
      return -1;
    }

    n += l + 1;
  }

  return n;
}

/* Parse SBP message payload into setting parameters */
settings_tokens_t settings_parse(const char *buf,
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
    return SETTINGS_TOKENS_INVALID;
  }

  if (buf[blen - 1] != '\0') {
    return SETTINGS_TOKENS_INVALID;
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
      if (name && (i + 1 < blen)) {
        *name = (const char *)&buf[i + 1];
      }
      break;

    case 2:
      if (value && (i + 1 < blen)) {
        *value = (const char *)&buf[i + 1];
      }
      break;

    case 3:
      if (type && (i + 1 < blen)) {
        *type = (const char *)&buf[i + 1];
        break;
      }

    case 4:
      /* Enum list sentinel NULL */
      if (i == blen - 2) break;

    case 5:
      if (i == blen - 1) break;

    default: return SETTINGS_TOKENS_INVALID;
    }
  }

  return tok;
}
