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

#include <libsettings/settings.h>

#include <internal/request_state.h>
#include <internal/setting_def.h>

#define LIBSETTINGS_LOCK(ctx)                            \
  do {                                                   \
    if ((ctx)->client_iface.lock) {                      \
      (ctx)->client_iface.lock((ctx)->client_iface.ctx); \
    }                                                    \
  } while (0)

#define LIBSETTINGS_UNLOCK(ctx)                            \
  do {                                                     \
    if ((ctx)->client_iface.unlock) {                      \
      (ctx)->client_iface.unlock((ctx)->client_iface.ctx); \
    }                                                      \
  } while (0)

/**
 * @brief request_state_init - set up compare structure for synchronous req/reply
 * @param ctx: settings context
 * @param msg_id: pending request type
 * @param data: formatted settings header string to match with incoming messages
 * @param data_len: length of match string
 */
void request_state_init(request_state_t *state,
                        void *event,
                        uint16_t msg_id,
                        const char *data,
                        size_t data_len)
{
  assert(!state->pending);

  memset(state, 0, sizeof(request_state_t));

  assert(data_len <= sizeof(state->compare_data));

  state->msg_id = msg_id;

  memcpy(state->compare_data, data, data_len);
  state->compare_data_len = data_len;
  state->match = false;
  state->pending = true;
  state->status = SETTINGS_WR_TIMEOUT;
  state->event = event;
}

/**
 * @brief request_state_check - used by message callbacks to perform comparison
 * @param ctx: settings context
 * @param data: settings message payload string to match with header string
 * @param data_len: length of payload string
 * @return 0 for match, 1 no comparison pending, -1 for comparison failure
 */
request_state_t *request_state_check(settings_t *ctx, const char *data, size_t data_len)
{
  assert(ctx);
  assert(data);
  assert(data_len > 0);

  request_state_t *state = request_state_lookup(ctx, data, data_len);

  if (NULL == state) {
    return NULL;
  }

  if (!state->pending) {
    return NULL;
  }

  return state;
}

/**
 * @brief request_state_match - returns status of current comparison
 * This is used as the value to block on until the comparison has been matched
 * successfully or until (based on implementation) a number of retries or a
 * timeout has expired.
 * @param ctx: settings context
 * @return true if response was matched, false if not response has been received
 */
bool request_state_match(const request_state_t *state)
{
  assert(state);

  return state->match;
}

int request_state_signal(request_state_t *state, settings_api_t *api, uint16_t msg_id)
{
  assert(state);
  if (msg_id != state->msg_id) {
    log_warn("message id mismatch");
    return -1;
  }

  state->match = true;
  state->pending = false;

  if (api->signal_thd && state->event) {
    api->signal_thd(state->event);
  } else {
    api->signal(api->ctx);
  }

  return 0;
}

/**
 * @brief request_state_deinit - clean up compare structure after transaction
 * @param ctx: settings context
 */
void request_state_deinit(request_state_t *state)
{
  state->pending = false;
}

void request_state_append(settings_t *ctx, request_state_t *state_data)
{
  LIBSETTINGS_LOCK(ctx);

  if (ctx->req_list == NULL) {
    ctx->req_list = state_data;
  } else {
    /* Find last element */
    request_state_t *last = ctx->req_list;
    while (last->next != NULL) {
      last = last->next;
    }
    last->next = state_data;
  }

  LIBSETTINGS_UNLOCK(ctx);
}

void request_state_remove(settings_t *ctx, request_state_t *state_data)
{
  assert(ctx != NULL);
  assert(state_data != NULL);

  LIBSETTINGS_LOCK(ctx);

  request_state_t *curr = ctx->req_list;
  request_state_t *prev = NULL;
  /* Find element to remove */
  while (curr != NULL) {
    if (curr != state_data) {
      prev = curr;
      curr = curr->next;
      continue;
    }

    if (prev == NULL) {
      /* This is the first item in the list make next one the new list head */
      ctx->req_list = curr->next;
    } else {
      prev->next = curr->next;
    }
    break;
  }

  LIBSETTINGS_UNLOCK(ctx);
}

request_state_t *request_state_lookup(settings_t *ctx, const char *data, size_t data_len)
{
  LIBSETTINGS_LOCK(ctx);

  request_state_t *state_list = ctx->req_list;
  while (state_list != NULL) {
    if ((data_len >= state_list->compare_data_len)
        && (memcmp(data, state_list->compare_data, state_list->compare_data_len) == 0)) {
      break;
    }
    state_list = state_list->next;
  }

  LIBSETTINGS_UNLOCK(ctx);

  return state_list;
}

void request_state_free(settings_t *ctx)
{
  LIBSETTINGS_LOCK(ctx);

  request_state_t *state_list = ctx->req_list;

  /* Free setting data list elements */
  while (state_list != NULL) {
    request_state_t *s = state_list;
    state_list = state_list->next;
    request_state_deinit(s);
    free(s);
  }

  ctx->req_list = NULL;

  LIBSETTINGS_UNLOCK(ctx);
}
