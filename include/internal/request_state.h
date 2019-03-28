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

#ifndef LIBSETTINGS_REGISTRATION_STATE_H
#define LIBSETTINGS_REGISTRATION_STATE_H

#include <libsettings/settings.h>

/**
 * @brief Registration Helper Struct
 *
 * This helper struct is used watch for async callbacks during the
 * registration/add watch read req phases of setup to allow a
 * synchronous blocking stragety. These are for ephemeral use.
 */
typedef struct request_state_s {
  bool pending;
  bool match;
  uint16_t msg_id;
  uint8_t compare_data[SETTINGS_BUFLEN];
  uint8_t compare_data_len;
  char resp_section[SETTINGS_BUFLEN];
  char resp_name[SETTINGS_BUFLEN];
  char resp_value[SETTINGS_BUFLEN];
  char resp_type[SETTINGS_BUFLEN];
  bool read_by_idx_done;
  settings_write_res_t status;
  void *event;
  struct request_state_s *next;
} request_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void request_state_init(request_state_t *state,
                        void *event,
                        uint16_t msg_id,
                        const char *data,
                        size_t data_len);
request_state_t *request_state_check(settings_t *ctx, const char *data, size_t data_len);
bool request_state_match(const request_state_t *state);
int request_state_signal(request_state_t *state, settings_api_t *api, uint16_t msg_id);
void request_state_deinit(request_state_t *state);

/* List functions */
void request_state_append(settings_t *ctx, request_state_t *state_data);
void request_state_remove(settings_t *ctx, request_state_t *state_data);
request_state_t *request_state_lookup(settings_t *ctx, const char *data, size_t data_len);
void request_state_free(request_state_t *state_list);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_REGISTRATION_STATE_H */
