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

int settings_format(const char *section, const char *name, const char *value,
                    const char *type, char *buf, size_t blen) {
  int n = 0;
  int l = 0;

  const char *tokens[] = {section, name, value, type};

  for (uint8_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); ++i) {
    const char *token = tokens[i];

    if (token == NULL) {
      break;
    }

    l = snprintf(&buf[n], blen - n, "%s", token);

    if ((l < 0) || ((size_t)l >= blen - n)) {
      return -1;
    }

    n += l + 1;
  }

  return n;
}

/* Parse SBP message payload into setting parameters */
settings_tokens_t settings_parse(const char *buf, size_t blen,
                                 const char **section, const char **name,
                                 const char **value, const char **type) {
  if (section) *section = NULL;
  if (name) *name = NULL;
  if (value) *value = NULL;
  if (type) *type = NULL;

  /* All strings must be NULL terminated */
  if ((blen > 0) && (buf[blen - 1] != '\0')) {
    return SETTINGS_TOKENS_INVALID;
  }

  const char **tokens[] = {section, name, value, type};
  settings_tokens_t tok = SETTINGS_TOKENS_EMPTY;
  size_t str_start = 0;
  for (size_t idx = 0; idx < blen; ++idx) {
    if (buf[idx] != '\0') {
      continue;
    }
    if ((size_t)tok < sizeof(tokens) / sizeof(tokens[0]) &&
        tokens[tok] != NULL) {
      *(tokens[tok]) = &buf[str_start];
    }
    str_start = idx + 1;
    tok++;
  }

  return (tok <= SETTINGS_TOKENS_EXTRA_NULL) ? tok : SETTINGS_TOKENS_INVALID;
}
