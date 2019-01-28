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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libsettings/settings_util.h>

#include <internal/setting_data.h>

setting_data_t *setting_data_create(type_data_t *type_list,
                                    const char *section,
                                    const char *name,
                                    void *var,
                                    size_t var_len,
                                    settings_type_t type,
                                    settings_notify_fn notify,
                                    void *notify_context,
                                    bool readonly,
                                    bool watchonly)
{
  /* Look up type data */
  type_data_t *type_data = type_data_lookup(type_list, type);
  assert(type_data != NULL);

  /* Set up setting data */
  setting_data_t *setting_data = (setting_data_t *)malloc(sizeof(*setting_data));
  if (setting_data == NULL) {
    return NULL;
  }

  *setting_data = (setting_data_t){
    .section = malloc(strlen(section) + 1),
    .name = malloc(strlen(name) + 1),
    .var = var,
    .var_len = var_len,
    .var_copy = malloc(var_len),
    .type_data = type_data,
    .notify = notify,
    .notify_context = notify_context,
    .readonly = readonly,
    .watchonly = watchonly,
    .next = NULL,
  };

  if ((setting_data->section == NULL) || (setting_data->name == NULL)
      || (setting_data->var_copy == NULL)) {
    setting_data_destroy(setting_data);
    free(setting_data);
    setting_data = NULL;
  } else {
    /* See setting_data initialization, section and name are guaranteed to fit */
    strcpy(setting_data->section, section);
    strcpy(setting_data->name, name);
  }

  return setting_data;
}

void setting_data_destroy(setting_data_t *setting_data)
{
  if (setting_data->section) {
    free(setting_data->section);
    setting_data->section = NULL;
  }

  if (setting_data->name) {
    free(setting_data->name);
    setting_data->name = NULL;
  }

  if (setting_data->var_copy) {
    free(setting_data->var_copy);
    setting_data->var_copy = NULL;
  }
}

settings_write_res_t setting_data_update_value(setting_data_t *setting_data,
                                               const char *value)
{
  if (setting_data->readonly) {
    return SETTINGS_WR_READ_ONLY;
  }

  /* Store copy and update value */
  memcpy(setting_data->var_copy, setting_data->var, setting_data->var_len);
  if (!setting_data->type_data->from_string(setting_data->type_data->priv,
                                            setting_data->var,
                                            setting_data->var_len,
                                            value)) {
    /* Revert value if conversion fails */
    memcpy(setting_data->var, setting_data->var_copy, setting_data->var_len);
    return SETTINGS_WR_PARSE_FAILED;
  }

  if (NULL == setting_data->notify) {
    return SETTINGS_WR_OK;
  }

  /* Call notify function */
  settings_write_res_t res = setting_data->notify(setting_data->notify_context);

  if (setting_data->watchonly) {
    /* No need for actions */
    return SETTINGS_WR_OK;
  }

  if (res != SETTINGS_WR_OK) {
    /* Revert value if notify returns error */
    memcpy(setting_data->var, setting_data->var_copy, setting_data->var_len);
  }

  return res;
}

int setting_data_format(setting_data_t *setting_data,
                        bool type,
                        char *buf,
                        int blen,
                        uint8_t *msg_hdr_len)
{
  int res = 0;
  int bytes = 0;

  res = settings_format(setting_data->section, setting_data->name, NULL, NULL, buf, blen);

  if (res <= 0) {
    return -1;
  }
  bytes += res;

  if (msg_hdr_len != NULL) {
    *msg_hdr_len = res;
  }

  /* Value */
  res = setting_data->type_data->to_string(setting_data->type_data->priv,
                                           &buf[bytes],
                                           blen - bytes,
                                           setting_data->var,
                                           setting_data->var_len);
  if ((res < 0) || (res >= blen - bytes)) {
    return -1;
  }
  /* Add terminating null (+ 1) */
  bytes += res + 1;

  if (!type) {
    return bytes;
  }

  /* Type information */
  if (setting_data->type_data->format_type == NULL) {
    return bytes;
  }

  res =
    setting_data->type_data->format_type(setting_data->type_data->priv, &buf[bytes], blen - bytes);
  if ((res < 0) || (res >= blen - bytes)) {
    return -1;
  }
  /* Add terminating null (+ 1) */
  bytes += res + 1;

  return bytes;
}

void setting_data_append(setting_data_t **data_list, setting_data_t *setting_data)
{
  if (*data_list == NULL) {
    *data_list = setting_data;
  } else {
    setting_data_t *s;
    /* Find last element in the same section */
    for (s = *data_list; s->next != NULL; s = s->next) {
      if ((strcmp(s->section, setting_data->section) == 0)
          && (strcmp(s->next->section, setting_data->section) != 0)) {
        break;
      }
    }
    setting_data->next = s->next;
    s->next = setting_data;
  }
}

void setting_data_remove(setting_data_t *data_list, setting_data_t **setting_data)
{
  if (data_list == NULL) {
    return;
  }

  setting_data_t *s;
  /* Find element before the one to remove */
  for (s = data_list; s->next != NULL; s = s->next) {
    if (s->next != *setting_data) {
      continue;
    }

    struct setting_data_s *to_be_freed = s->next;

    *setting_data = NULL;
    s->next = s->next->next;

    setting_data_destroy(to_be_freed);
    free(to_be_freed);
  }
}

setting_data_t *setting_data_lookup(setting_data_t *data_list,
                                    const char *section,
                                    const char *name)
{
  while (data_list != NULL) {
    if ((strcmp(data_list->section, section) == 0) && (strcmp(data_list->name, name) == 0)) {
      break;
    }
    data_list = data_list->next;
  }
  return data_list;
}

void setting_data_free(setting_data_t *data_list)
{
  /* Free setting data list elements */
  while (data_list != NULL) {
    setting_data_t *s = data_list;
    data_list = data_list->next;
    setting_data_destroy(s);
    free(s);
  }
}
