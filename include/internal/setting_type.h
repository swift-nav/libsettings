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

#ifndef LIBSETTINGS_SETTING_TYPE_H
#define LIBSETTINGS_SETTING_TYPE_H

#include <stdbool.h>

#include <libsettings/settings.h>

typedef int (*to_string_fn)(const void *priv, char *str, int slen,
                            const void *blob, int blen);
typedef bool (*from_string_fn)(const void *priv, void *blob, int blen,
                               const char *str);
typedef int (*format_type_fn)(const void *priv, char *str, int slen);

/**
 * @brief Type Data
 *
 * This structure encapsulates the codec for values of a given type
 * which the settings context uses to build a list of known types
 * that it can support when settings are added to the settings list.
 */
typedef struct type_data_s {
  to_string_fn to_string;
  from_string_fn from_string;
  format_type_fn format_type;
  const void *priv;
  struct type_data_s *next;
} type_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief type_data_lookup - retrieves type node from settings context
 * @param data_list: type data list
 * @param type: type struct to be matched
 * @return the setting type entry if a match is found, otherwise NULL
 */
type_data_t *type_data_lookup(type_data_t *data_list, settings_type_t type);

/**
 * @brief type_register - register type data for reference when adding settings
 * @param data_list: pointer to type data list
 * @param to_string: serialization method
 * @param from_string: deserialization method
 * @param format_type: ?
 * @param priv: private data used in ser/des methods
 * @param type: type enum that is used to identify this type
 * @return
 */
int type_register(type_data_t **data_list, to_string_fn to_string,
                  from_string_fn from_string, format_type_fn format_type,
                  const void *priv, settings_type_t *type);

/**
 * @brief type_data_free - free type data list
 * @param data_list: type data list to free
 */
void type_data_free(type_data_t *data_list);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTING_TYPE_H */
