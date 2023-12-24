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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libsbp/legacy/settings.h>

#include <swiftnav/logging.h>

#include <libsettings/settings_util.h>

#include <internal/request_state.h>
#include <internal/setting_data.h>
#include <internal/setting_def.h>
#include <internal/setting_macros.h>
#include <internal/setting_sbp_cb.h>

#define SBP_SENDER_ID 0x42

#define UPDATE_FILTER_NONE 0x0
#define UPDATE_FILTER_BASIC (0x1 << 3)
#define UPDATE_FILTER_READONLY (0x1 << 1)
#define UPDATE_FILTER_WATCHONLY (0x1 << 2)

#define update_filter_check(filter_mask, filter_enum) \
  (filter_mask & filter_enum)

/**
 * @brief setting_send_write_response
 * @param write_response: pre-formatted write response sbp message
 * @param len: length of the message
 * @return zero on success, -1 if message failed to send
 */
static int setting_send_write_response(settings_t *ctx,
                                       setting_data_t *setting_data,
                                       settings_write_res_t write_result) {
  uint8_t resp[SETTINGS_BUFLEN] = {0};
  uint8_t resp_len = 0;
  msg_settings_write_resp_t *write_response = (msg_settings_write_resp_t *)resp;
  write_response->status = write_result;
  resp_len += sizeof(write_response->status);
  int l = setting_data_format(setting_data, false, write_response->setting,
                              sizeof(resp) - resp_len, NULL);
  if (l < 0) {
    log_error("formatting settings write response failed");
    return -1;
  }
  resp_len += l;

  settings_api_t *api = &ctx->client_iface;
  if (api->send(api->ctx, SBP_MSG_SETTINGS_WRITE_RESP, resp_len,
                (uint8_t *)write_response) != 0) {
    log_error("sending settings write response failed");
    return -1;
  }
  return 0;
}

static void setting_update_value(settings_t *ctx, const char *msg, uint8_t len,
                                 uint32_t filter) {
  /* Extract parameters from message:
   * 4 null terminated strings: section, name, value and type.
   * Expect to find at least section, name and value.
   */
  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;
  if (settings_parse((char *)msg, len, &section, &name, &value, &type) <
      SETTINGS_TOKENS_VALUE) {
    log_warn("setting update value, error parsing setting");
    return;
  }

  /* Look up setting data */
  setting_data_t *setting_data =
      setting_data_lookup(ctx->setting_data_list, section, name);
  if (setting_data == NULL) {
    return;
  }

  if (update_filter_check(filter, UPDATE_FILTER_WATCHONLY) &&
      setting_data->watchonly) {
    return;
  }

  if (update_filter_check(filter, UPDATE_FILTER_READONLY) &&
      setting_data->readonly) {
    return;
  }

  if (update_filter_check(filter, UPDATE_FILTER_BASIC) &&
      !setting_data->readonly && !setting_data->watchonly) {
    return;
  }

  /* Reject messages that are too large for READ_BY_INDEX_RESP*/
  if (len > MAX_SETTING_WRITE_LEN) {
    setting_send_write_response(ctx, setting_data, SETTINGS_WR_VALUE_REJECTED);
    log_warn("setting message rejected, length:%u limit:%u", len,
             MAX_SETTING_WRITE_LEN);
    return;
  }

  settings_write_res_t write_result =
      setting_data_update_value(setting_data, value);

  /* In case of watcher, do not send write response */
  if (!setting_data->watchonly) {
    setting_send_write_response(ctx, setting_data, write_result);
  }
}

static void setting_register_resp_callback(uint16_t sender_id, uint8_t len,
                                           uint8_t *msg, void *context) {
  settings_t *ctx = (settings_t *)context;
  (void)sender_id;

  if (sender_id != SBP_SENDER_ID) {
    log_warn("invalid sender %d != %d", sender_id, SBP_SENDER_ID);
    return;
  }

  msg_settings_register_resp_t *resp = (msg_settings_register_resp_t *)msg;

  switch ((settings_reg_res_t)resp->status) {
    case SETTINGS_REG_PARSE_FAILED:
      /* In case the request was corrupted during transfer, let the timeout
       * trigger and request be sent again, parse error is printed in
       * sbp_settings_daemon */
      return;

    /* Use the returned value to update in all cases */
    case SETTINGS_REG_OK:
    case SETTINGS_REG_OK_PERM:
    case SETTINGS_REG_REGISTERED:
      break;

    default:
      log_error("invalid reg resp return code %d", resp->status);
      return;
  }

  /* Check for a response to a pending registration request */
  request_state_t *state =
      request_state_check(ctx, resp->setting, len - sizeof(resp->status));

  if (NULL == state) {
    /* No pending registration request or no pending request and the response
     * don't match, most likely this response was meant to some other client
     * doing registration at the same time. */
    return;
  }

  /* In case of readonly, trust the initialized value.
   * Watchers shall not be readonly. */
  setting_update_value(ctx, resp->setting, len - sizeof(resp->status),
                       UPDATE_FILTER_READONLY);

  request_state_signal(state, &ctx->client_iface, SBP_MSG_SETTINGS_REGISTER);
}

static void setting_write_callback(uint16_t sender_id, uint8_t len,
                                   uint8_t *msg, void *context) {
  settings_t *ctx = (settings_t *)context;
  (void)sender_id;

  if (sender_id != SBP_SENDER_ID) {
    log_warn("invalid sender %d != %d", sender_id, SBP_SENDER_ID);
    return;
  }

  msg_settings_write_t *request = (msg_settings_write_t *)msg;

  /* Update value, ignore watchers, they are updated from
   * setting_write_resp_callback() */
  setting_update_value(ctx, request->setting, len, UPDATE_FILTER_WATCHONLY);
}

static void setting_read_resp_callback(uint16_t sender_id, uint8_t len,
                                       uint8_t *msg, void *context) {
  (void)sender_id;
  assert(msg);
  assert(context);

  settings_t *ctx = (settings_t *)context;
  const msg_settings_read_resp_t *read_response =
      (msg_settings_read_resp_t *)msg;

  /* Check for a response to a pending request */
  request_state_t *state =
      request_state_check(ctx, read_response->setting, len);

  if (NULL == state) {
    return;
  }

  state->resp_value_valid = false;
  state->resp_value[0] = '\0';
  state->resp_type[0] = '\0';

  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;
  settings_tokens_t tokens = settings_parse(read_response->setting, len,
                                            &section, &name, &value, &type);
  if (tokens >= SETTINGS_TOKENS_VALUE) {
    if (value) {
      strncpy(state->resp_value, value, sizeof(state->resp_value) - 1);
      state->resp_value[sizeof(state->resp_value) - 1] = '\0';
      state->resp_value_valid = true;
    }
    if (type) {
      strncpy(state->resp_type, type, sizeof(state->resp_type) - 1);
      state->resp_type[sizeof(state->resp_type) - 1] = '\0';
    }
  } else if (tokens == SETTINGS_TOKENS_NAME) {
    log_debug("setting %s.%s not found", section, name);
  } else {
    log_warn("read response parsing failed");
  }

  request_state_signal(state, &ctx->client_iface, SBP_MSG_SETTINGS_READ_REQ);
}

static void setting_write_resp_callback(uint16_t sender_id, uint8_t len,
                                        uint8_t *msg, void *context) {
  (void)sender_id;
  settings_t *ctx = (settings_t *)context;
  msg_settings_write_resp_t *write_response = (msg_settings_write_resp_t *)msg;

  if (write_response->status == SETTINGS_WR_OK) {
    /* Update watchers, do not update the actual setting since it's already done
     * in setting_write_callback() */
    setting_update_value(ctx, write_response->setting,
                         len - sizeof(write_response->status),
                         UPDATE_FILTER_BASIC);
  }

  /* Check for a response to a pending request */
  request_state_t *state = request_state_check(
      ctx, write_response->setting, len - sizeof(write_response->status));

  if (NULL == state) {
    return;
  }

  state->status = write_response->status;

  request_state_signal(state, &ctx->client_iface, SBP_MSG_SETTINGS_WRITE);
}

static void setting_read_by_index_resp_callback(uint16_t sender_id, uint8_t len,
                                                uint8_t *msg, void *context) {
  (void)sender_id;
  settings_t *ctx = (settings_t *)context;
  msg_settings_read_by_index_resp_t *resp =
      (msg_settings_read_by_index_resp_t *)msg;

  /* Check for a response to a pending request */
  request_state_t *state =
      request_state_check(ctx, (char *)msg, sizeof(resp->index));

  if (NULL == state) {
    return;
  }

  state->resp_value_valid = false;
  state->resp_section[0] = '\0';
  state->resp_name[0] = '\0';
  state->resp_value[0] = '\0';
  state->resp_type[0] = '\0';

  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;
  if (settings_parse(resp->setting, len - sizeof(resp->index), &section, &name,
                     &value, &type) > 0) {
    if (section) {
      strncpy(state->resp_section, section, sizeof(state->resp_section) - 1);
      state->resp_section[sizeof(state->resp_section) - 1] = '\0';
    }
    if (name) {
      strncpy(state->resp_name, name, sizeof(state->resp_name) - 1);
      state->resp_name[sizeof(state->resp_name) - 1] = '\0';
    }
    if (value) {
      strncpy(state->resp_value, value, sizeof(state->resp_value) - 1);
      state->resp_value[sizeof(state->resp_value) - 1] = '\0';
      state->resp_value_valid = true;
    }
    if (type) {
      strncpy(state->resp_type, type, sizeof(state->resp_type) - 1);
      state->resp_type[sizeof(state->resp_type) - 1] = '\0';
    }
  }

  request_state_signal(state, &ctx->client_iface,
                       SBP_MSG_SETTINGS_READ_BY_INDEX_REQ);
}

static void setting_read_by_index_done_callback(uint16_t sender_id, uint8_t len,
                                                uint8_t *msg, void *context) {
  (void)sender_id;
  (void)len;
  (void)msg;

  settings_t *ctx = (settings_t *)context;

  /* Traverse the pending requests */
  request_state_t *state = ctx->req_list;
  while (state != NULL) {
    state->read_by_idx_done = true;
    request_state_signal(state, &ctx->client_iface,
                         SBP_MSG_SETTINGS_READ_BY_INDEX_REQ);
    state = state->next;
  }
}

static sbp_msg_callback_t setting_sbp_cb_get(uint16_t msg_id) {
  switch (msg_id) {
    case SBP_MSG_SETTINGS_REGISTER_RESP:
      return setting_register_resp_callback;

    case SBP_MSG_SETTINGS_WRITE:
      return setting_write_callback;

    case SBP_MSG_SETTINGS_WRITE_RESP:
      return setting_write_resp_callback;

    case SBP_MSG_SETTINGS_READ_RESP:
      return setting_read_resp_callback;

    case SBP_MSG_SETTINGS_READ_BY_INDEX_RESP:
      return setting_read_by_index_resp_callback;

    case SBP_MSG_SETTINGS_READ_BY_INDEX_DONE:
      return setting_read_by_index_done_callback;

    default:
      assert(!"callback not found");
  }

  /* To make cythonizer happy.. */
  return NULL;
}

int setting_sbp_cb_register(settings_t *ctx, uint16_t msg_id) {
  LIBSETTINGS_LOCK(ctx);
  setting_sbp_cb_t *sbp_cb_list = ctx->sbp_cb_list;
  setting_sbp_cb_t *last = NULL;

  /* Traverse to list end */
  while (sbp_cb_list != NULL) {
    if (sbp_cb_list->msg_id == msg_id) {
      /* Already registered */
      LIBSETTINGS_UNLOCK(ctx);
      return 1;
    }
    last = sbp_cb_list;
    sbp_cb_list = sbp_cb_list->next;
  }

  setting_sbp_cb_t *sbp_cb = malloc(sizeof(*sbp_cb));
  if (sbp_cb == NULL) {
    LIBSETTINGS_UNLOCK(ctx);
    return -1;
  }

  *sbp_cb = (setting_sbp_cb_t){.msg_id = msg_id,
                               .cb = setting_sbp_cb_get(msg_id),
                               .cb_node = NULL,
                               .next = NULL};

  int res = ctx->client_iface.register_cb(ctx->client_iface.ctx, sbp_cb->msg_id,
                                          sbp_cb->cb, ctx, &sbp_cb->cb_node);

  if (res != 0) {
    log_error("error registering callback for msg id %d", msg_id);
    free(sbp_cb);
    LIBSETTINGS_UNLOCK(ctx);
    return -1;
  }

  if (last == NULL) {
    /* List was empty, set as list head */
    ctx->sbp_cb_list = sbp_cb;
  } else {
    last->next = sbp_cb;
  }

  LIBSETTINGS_UNLOCK(ctx);
  return 0;
}

int setting_sbp_cb_unregister(settings_t *ctx, uint16_t msg_id) {
  LIBSETTINGS_LOCK(ctx);
  if (ctx->sbp_cb_list == NULL) {
    /* List is empty, nothing to unregister */
    LIBSETTINGS_UNLOCK(ctx);
    return 1;
  }

  setting_sbp_cb_t *sbp_cb_list = ctx->sbp_cb_list;

  setting_sbp_cb_t *prev = NULL;
  while (sbp_cb_list != NULL) {
    if (sbp_cb_list->msg_id == msg_id) {
      /* Registration found */
      break;
    }
    prev = sbp_cb_list;
    sbp_cb_list = sbp_cb_list->next;
  }

  if (sbp_cb_list == NULL) {
    /* Not registered */
    LIBSETTINGS_UNLOCK(ctx);
    return 1;
  }

  setting_sbp_cb_t *sbp_cb = sbp_cb_list;

  /* Update list linking */
  if (prev == NULL) {
    /* This was first in the list, update list head */
    ctx->sbp_cb_list = sbp_cb->next;
  } else {
    prev->next = sbp_cb->next;
  }

  int ret = 0;

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx,
                                      &sbp_cb->cb_node) != 0) {
    log_error("error unregistering callback for msg id %d", msg_id);
    ret = -1;
  }

  free(sbp_cb);
  LIBSETTINGS_UNLOCK(ctx);
  return ret;
}
