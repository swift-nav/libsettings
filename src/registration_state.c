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
#include <string.h>

#include <libsettings/settings.h>

#include <internal/registration_state.h>

/**
 * @brief registration_state_init - set up compare structure for synchronous
 * req/reply
 * @param ctx: settings context
 * @param data: formatted settings header string to match with incoming messages
 * @param data_len: length of match string
 */
void registration_state_init(registration_state_t *state, const char *data, size_t data_len)
{
  /* No multithreading */
  assert(!state->pending);

  memset(state, 0, sizeof(registration_state_t));

  assert(data_len <= sizeof(state->compare_data));

  memcpy(state->compare_data, data, data_len);
  state->compare_data_len = data_len;
  state->match = false;
  state->pending = true;
}

/**
 * @brief registration_state_match - returns status of current comparison
 * This is used as the value to block on until the comparison has been matched
 * successfully or until (based on implementation) a number of retries or a
 * timeout has expired.
 * @param ctx: settings context
 * @return true if response was matched, false if not response has been received
 */
bool registration_state_match(registration_state_t *state)
{
  return state->match;
}

/**
 * @brief registration_state_check - used by message callbacks to perform
 * comparison
 * @param ctx: settings context
 * @param data: settings message payload string to match with header string
 * @param data_len: length of payload string
 * @return 0 for match, 1 no comparison pending, -1 for comparison failure
 */
int registration_state_check(registration_state_t *state,
                             settings_api_t *api,
                             const char *data,
                             size_t data_len)
{
  assert(api);
  assert(state);
  assert(data);

  if (!state->pending) {
    return 1;
  }

  if ((data_len >= state->compare_data_len)
      && (memcmp(data, state->compare_data, state->compare_data_len) == 0)) {
    state->match = true;
    state->pending = false;
    api->signal(api->ctx);
    return 0;
  }

  return -1;
}

/**
 * @brief registration_state_deinit - clean up compare structure after
 * transaction
 * @param ctx: settings context
 */
void registration_state_deinit(registration_state_t *state)
{
  state->pending = false;
}
