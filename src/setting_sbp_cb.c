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

#include <libsbp/settings.h>

#include <libsettings/settings_util.h>

#include <internal/request_state.h>
#include <internal/setting_data.h>
#include <internal/setting_def.h>
#include <internal/setting_sbp_cb.h>

/**
 * @brief setting_send_write_response
 * @param write_response: pre-formatted write response sbp message
 * @param len: length of the message
 * @return zero on success, -1 if message failed to send
 */
static int setting_send_write_response(settings_t *ctx,
                                       msg_settings_write_resp_t *write_response,
                                       uint8_t len)
{
  settings_api_t *api = &ctx->client_iface;
  if (api->send(api->ctx, SBP_MSG_SETTINGS_WRITE_RESP, len, (uint8_t *)write_response) != 0) {
    api->log(log_err, "sending settings write response failed");
    return -1;
  }
  return 0;
}

static void setting_update_value(settings_t *ctx, uint8_t *msg, uint8_t len)
{
  /* Extract parameters from message:
   * 4 null terminated strings: section, name, value and type.
   * Expect to find at least section, name and value.
   */
  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;
  if (settings_parse((char *)msg, len, &section, &name, &value, &type) < SETTINGS_TOKENS_VALUE) {
    ctx->client_iface.log(log_warning, "setting update value, error parsing setting");
    return;
  }

  /* Look up setting data */
  setting_data_t *setting_data = setting_data_lookup(ctx->setting_data_list, section, name);
  if (setting_data == NULL) {
    return;
  }

  if (setting_data->watchonly) {
    return;
  }

  settings_write_res_t write_result = setting_data_update_value(setting_data, value);

  uint8_t resp[SETTINGS_BUFLEN] = {0};
  uint8_t resp_len = 0;
  msg_settings_write_resp_t *write_response = (msg_settings_write_resp_t *)resp;
  write_response->status = write_result;
  resp_len += sizeof(write_response->status);
  int l = setting_data_format(setting_data,
                              false,
                              write_response->setting,
                              sizeof(resp) - resp_len,
                              NULL);
  if (l < 0) {
    return;
  }
  resp_len += l;

  setting_send_write_response(ctx, write_response, resp_len);
}

static void setting_register_resp_callback(uint16_t sender_id, uint8_t len, uint8_t *msg, void *context)
{
  settings_t *ctx = (settings_t *)context;
  (void)sender_id;

  if (sender_id != SBP_SENDER_ID) {
    ctx->client_iface.log(log_warning, "invalid sender %d != %d", sender_id, SBP_SENDER_ID);
    return;
  }

  msg_settings_register_resp_t *resp = (msg_settings_register_resp_t *)msg;

  if (resp->status == SETTINGS_REG_PARSE_FAILED) {
    /* In case the request was corrupted during transfer, let the timeout trigger
     * and request be sent again, parse error is printed in sbp_settings_daemon */
    return;
  }

  /* Check for a response to a pending registration request */
  int state = request_state_check(&ctx->request_state, &ctx->client_iface, resp->setting, len - sizeof(resp->status));

  if (state > 0) {
    /* No pending registration request */
    return;
  }

  if (state < 0) {
    ctx->client_iface.log(log_warning, "settings register resp cb, register req/resp mismatch");
    return;
  }

  setting_update_value(ctx, (uint8_t *)resp->setting, len - sizeof(resp->status));
}

static void setting_write_callback(uint16_t sender_id, uint8_t len, uint8_t *msg, void *context)
{
  settings_t *ctx = (settings_t *)context;
  (void)sender_id;

  if (sender_id != SBP_SENDER_ID) {
    ctx->client_iface.log(log_warning, "invalid sender %d != %d", sender_id, SBP_SENDER_ID);
    return;
  }

  setting_update_value(ctx, msg, len);
}

static int setting_update_watch_only(settings_t *ctx, const char *msg, uint8_t len)
{
  /* Extract parameters from message:
   * 4 null terminated strings: section, name, value and type
   * Expect to find at least section, name and value.
   */
  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;
  if (settings_parse(msg, len, &section, &name, &value, &type) < SETTINGS_TOKENS_VALUE) {
    ctx->client_iface.log(log_warning, "updating watched values failed, error parsing setting");
    return -1;
  }

  /* Look up setting data */
  setting_data_t *setting_data = setting_data_lookup(ctx->setting_data_list, section, name);
  if (setting_data == NULL) {
    return 0;
  }

  if (!setting_data->watchonly) {
    return 0;
  }

  if (setting_data_update_value(setting_data, value) != SETTINGS_WR_OK) {
    return -1;
  }

  return 0;
}

static void setting_read_resp_callback(uint16_t sender_id,
                                        uint8_t len,
                                        uint8_t msg[],
                                        void *context)
{
  (void)sender_id;
  assert(msg);
  assert(context);

  settings_t *ctx = (settings_t *)context;
  const msg_settings_read_resp_t *read_response = (msg_settings_read_resp_t *)msg;

  /* Check for a response to a pending request */
  int res =
    request_state_check(&ctx->request_state, &ctx->client_iface, read_response->setting, len);

  if (res != 0) {
    return;
  }

  if (setting_update_watch_only(ctx, read_response->setting, len) != 0) {
    ctx->client_iface.log(log_warning, "error in settings read response message");
  }

  const char *value = NULL, *type = NULL;
  if (settings_parse(read_response->setting, len, NULL, NULL, &value, &type)
      >= SETTINGS_TOKENS_VALUE) {
    if (value) {
      strncpy(ctx->resp_value, value, sizeof(ctx->resp_value));
    }
    if (type) {
      strncpy(ctx->resp_type, type, sizeof(ctx->resp_type));
    }
  } else {
    ctx->client_iface.log(log_warning, "read response parsing failed");
    ctx->resp_value[0] = '\0';
    ctx->resp_type[0] = '\0';
  }
}

static void setting_write_resp_callback(uint16_t sender_id,
                                         uint8_t len,
                                         uint8_t msg[],
                                         void *context)
{
  (void)sender_id;
  settings_t *ctx = (settings_t *)context;
  msg_settings_write_resp_t *write_response = (msg_settings_write_resp_t *)msg;

  ctx->status = write_response->status;

  /* Check for a response to a pending request */
  request_state_check(&ctx->request_state,
                      &ctx->client_iface,
                      write_response->setting,
                      len - sizeof(write_response->status));

  if (write_response->status != SETTINGS_WR_OK) {
    ctx->client_iface.log(log_warning,
                          "setting write rejected (code: %d), not updating watched values",
                          write_response->status);
    return;
  }

  if (setting_update_watch_only(ctx, write_response->setting, len - sizeof(write_response->status))
      != 0) {
    ctx->client_iface.log(log_warning, "error in settings read response message");
  }
}

static void setting_read_by_index_resp_callback(uint16_t sender_id,
                                                 uint8_t len,
                                                 uint8_t msg[],
                                                 void *context)
{
  (void)sender_id;
  settings_t *ctx = (settings_t *)context;
  msg_settings_read_by_index_resp_t *resp = (msg_settings_read_by_index_resp_t *)msg;

  /* Check for a response to a pending request */
  int res =
    request_state_check(&ctx->request_state, &ctx->client_iface, (char *)msg, sizeof(resp->index));

  if (res != 0) {
    return;
  }

  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;

  if (settings_parse(resp->setting, len - sizeof(resp->index), &section, &name, &value, &type)
      > 0) {
    if (section) {
      strncpy(ctx->resp_section, section, sizeof(ctx->resp_section));
    }
    if (name) {
      strncpy(ctx->resp_name, name, sizeof(ctx->resp_name));
    }
    if (value) {
      strncpy(ctx->resp_value, value, sizeof(ctx->resp_value));
    }
    if (type) {
      strncpy(ctx->resp_type, type, sizeof(ctx->resp_type));
    }
  }
}

static void setting_read_by_index_done_callback(uint16_t sender_id,
                                                 uint8_t len,
                                                 uint8_t msg[],
                                                 void *context)
{
  (void)sender_id;
  (void)len;
  (void)msg;

  settings_t *ctx = (settings_t *)context;

  ctx->resp_section[0] = '\0';
  ctx->resp_name[0] = '\0';
  ctx->resp_value[0] = '\0';
  ctx->resp_type[0] = '\0';
  ctx->read_by_idx_done = true;

  int ret = request_state_signal(&ctx->request_state,
                                 &ctx->client_iface,
                                 SBP_MSG_SETTINGS_READ_BY_INDEX_REQ);

  if (ret != 0) {
    ctx->client_iface.log(log_warning, "Signaling request state failed with code: %d", ret);
  }
}

static sbp_msg_callback_t setting_sbp_cb_get(uint16_t msg_id)
{
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

int setting_sbp_cb_register(settings_t *ctx, uint16_t msg_id)
{
  setting_sbp_cb_t *sbp_cb_list = ctx->sbp_cb_list;
  setting_sbp_cb_t *last = NULL;

  /* Traverse to list end */
  while (sbp_cb_list != NULL) {
    if (sbp_cb_list->msg_id == msg_id) {
      /* Already registered */
      return 1;
    }
    last = sbp_cb_list;
    sbp_cb_list = sbp_cb_list->next;
  }

  setting_sbp_cb_t *sbp_cb = malloc(sizeof(*sbp_cb));
  if (sbp_cb == NULL) {
    return -1;
  }

  *sbp_cb = (setting_sbp_cb_t){.msg_id = msg_id,
                               .cb = setting_sbp_cb_get(msg_id),
                               .cb_node = NULL,
                               .next = NULL};

  int res = ctx->client_iface.register_cb(ctx->client_iface.ctx,
                                          sbp_cb->msg_id,
                                          sbp_cb->cb,
                                          ctx,
                                          &sbp_cb->cb_node);

  if (res != 0) {
    ctx->client_iface.log(log_err, "error registering callback for msg id %d", msg_id);
    free(sbp_cb);
    return -1;
  }

  if (last == NULL) {
    /* List was empty, set as list head */
    ctx->sbp_cb_list = sbp_cb;
  } else {
    last->next = sbp_cb;
  }

  return 0;
}

int setting_sbp_cb_unregister(settings_t *ctx, uint16_t msg_id)
{
  if (ctx->sbp_cb_list == NULL) {
    /* List is empty, nothing to unregister */
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
    return 1;
  }

  setting_sbp_cb_t *sbp_cb = sbp_cb_list;
  sbp_msg_callbacks_node_t *cb_node = sbp_cb->cb_node;

  /* Update list linking */
  if (prev == NULL) {
    /* This was first in the list, update list head */
    ctx->sbp_cb_list = sbp_cb->next;
  } else {
    prev->next = sbp_cb->next;
  }

  free(sbp_cb);

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx, &cb_node) != 0) {
    ctx->client_iface.log(log_err, "error unregistering callback");
    return -1;
  }

  return 0;
}
