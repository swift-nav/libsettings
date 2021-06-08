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

#include <stdlib.h>

#include <internal/setting_type.h>

type_data_t *type_data_lookup(type_data_t *data_list, settings_type_t type) {
  for (int i = 0; i < type && data_list != NULL; i++) {
    data_list = data_list->next;
  }
  return data_list;
}

int type_register(type_data_t **data_list, to_string_fn to_string,
                  from_string_fn from_string, format_type_fn format_type,
                  const void *priv, settings_type_t *type) {
  type_data_t *type_data = (type_data_t *)malloc(sizeof(*type_data));
  if (type_data == NULL) {
    return -1;
  }

  *type_data = (type_data_t){.to_string = to_string,
                             .from_string = from_string,
                             .format_type = format_type,
                             .priv = priv,
                             .next = NULL};

  /* Add to list */
  settings_type_t next_type = 0;
  while (*data_list != NULL) {
    data_list = &(*data_list)->next;
    next_type++;
  }

  *data_list = type_data;
  *type = next_type;
  return 0;
}

void type_data_free(type_data_t *data_list) {
  /* Free type data list elements */
  while (data_list != NULL) {
    type_data_t *t = data_list;
    data_list = data_list->next;
    free(t);
  }
}
