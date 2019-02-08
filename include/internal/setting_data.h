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

#ifndef LIBSETTINGS_SETTING_DATA_H
#define LIBSETTINGS_SETTING_DATA_H

#include <internal/setting_type.h>

/**
 * @brief Setting Data
 *
 * This structure holds the information use to serialize settings
 * information into sbp messages, as well as internal flags used
 * to evaluate sbp settings callback behavior managed within the
 * settings context.
 */
typedef struct setting_data_s {
  char *section;
  char *name;
  void *var;
  size_t var_len;
  void *var_copy;
  type_data_t *type_data;
  settings_notify_fn notify;
  void *notify_context;
  bool readonly;
  bool watchonly;
  struct setting_data_s *next;
} setting_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief setting_data_create - setting data constructor
 * @param type_list: pointer to type data list
 * @param section: section identifier
 * @param name: setting name
 * @param var: non-owning reference to location the data is stored
 * @param var_len: length of data storage
 * @param type: type identifier
 * @param notify: optional notification callback
 * @param notify_context: optional data reference to pass during notification
 * @param readonly: set to true to disable value updates
 * @param watchonly: set to true to indicate a non-owned setting watch
 * @return the newly created setting, NULL if failed
 */
setting_data_t *setting_data_create(type_data_t *type_list,
                                    const char *section,
                                    const char *name,
                                    void *var,
                                    size_t var_len,
                                    settings_type_t type,
                                    settings_notify_fn notify,
                                    void *notify_context,
                                    bool readonly,
                                    bool watchonly);

/**
 * @brief setting_data_destroy - deinit for settings data
 * @param setting_data: setting to deinit
 */
void setting_data_destroy(setting_data_t *setting_data);

/**
 * @brief setting_data_update_value - process value string and update internal data
 * on success
 * @param setting_data: setting to update
 * @param value: value string to evaluate
 * @return result of updating the value
 */
settings_write_res_t setting_data_update_value(setting_data_t *setting_data, const char *value);

/**
 * @brief setting_data_format - formats a fully formed setting message
 * payload
 * @param setting_data: the setting to format
 * @param type: flag to indicate if type data shall be formatted
 * @param buf: buffer to hold formatted setting string
 * @param len: length of the destination buffer
 * @param msg_hdr_len: length of the msg header
 * @return number of bytes written to the buffer, -1 in case of failure
 */
int setting_data_format(setting_data_t *setting_data,
                        bool type,
                        char *buf,
                        int blen,
                        uint8_t *msg_hdr_len);

/**
 * @brief setting_data_append - appends setting data to end of setting data list
 * @param data_list: setting data list
 * @param setting_data: data to append
 * @return
 */
void setting_data_append(setting_data_t **data_list, setting_data_t *setting_data);

/**
 * @brief setting_data_list_remove - remove a setting from the setting data list
 * @param data_list: setting data list
 * @param setting_data: setting to remove
 */
void setting_data_remove(setting_data_t **data_list, setting_data_t **setting_data);

/**
 * @brief setting_data_lookup - retrieves setting node from settings data list
 * @param data_list: setting data list
 * @param section: setting section string to match
 * @param name: setting name string to match
 * @return the setting type entry if a match is found, otherwise NULL
 */
setting_data_t *setting_data_lookup(setting_data_t *data_list,
                                    const char *section,
                                    const char *name);

/**
 * @brief setting_data_free - free the setting data list
 * @param data_list: setting data list to free
 */
void setting_data_free(setting_data_t *data_list);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTING_DATA_H */
