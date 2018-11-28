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

#define SBP_PAYLOAD_SIZE_MAX 255

/**
 * @brief Registration Helper Struct
 *
 * This helper struct is used watch for async callbacks during the
 * registration/add watch read req phases of setup to allow a
 * synchronous blocking stragety. These are for ephemeral use.
 */
typedef struct {
  bool pending;
  bool match;
  uint16_t msg_id;
  uint8_t compare_data[SBP_PAYLOAD_SIZE_MAX];
  uint8_t compare_data_len;
} registration_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void request_state_init(request_state_t *state, uint16_t msg_id, const char *data, size_t data_len);
int request_state_check(request_state_t *state, settings_api_t *api, const char *data, size_t data_len);
bool request_state_match(const request_state_t *state);
int request_state_signal(request_state_t *state, settings_api_t *api, uint16_t msg_id);
void request_state_deinit(request_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_REGISTRATION_STATE_H */
