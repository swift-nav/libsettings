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
  uint8_t compare_data[SBP_PAYLOAD_SIZE_MAX];
  uint8_t compare_data_len;
} registration_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void registration_state_init(registration_state_t *state, const char *data, size_t data_len);
bool registration_state_match(registration_state_t *state);
int registration_state_check(registration_state_t *state, settings_api_t *api, const char *data, size_t data_len);
void registration_state_deinit(registration_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_REGISTRATION_STATE_H */