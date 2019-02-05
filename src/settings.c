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

/**
 * @file settings.c
 * @brief Implementation of Settings Client APIs
 *
 * The piksi settings daemon acts as the manager for onboard settings
 * registration and read responses for read requests while individual
 * processes are responsible for the ownership of settings values and
 * must respond to write requests with the value of the setting as a
 * response to the question of whether or not a write request was
 * valid and accepted by that process.
 *
 * This module provides a context and APIs for client interactions
 * with the settings manager. Where previously these APIs were intended
 * only for settings owning processes to register settings and respond
 * to write requests, the intention will be to allow a fully formed
 * settings client to be built upon these APIs to include read requests
 * and other client side queries.
 *
 * The high level approach to the client is to hold a list of unique
 * settings that can be configured as owned or non-owned (watch-only)
 * by the process running the client. Each setting which is added to
 * the list will be kept in sync with the settings daemon and/or the
 * owning process via asynchronous messages received in the routed
 * endpoint for the client.
 *
 * Standard usage is as follow, initialize the settings context:
 * \code{.c}
 * // Create the settings context
 * settings_t *settings_ctx = settings_create();
 * \endcode
 * Add a reader to the main pk_loop (if applicable)
 * \code{.c}
 * // Depending on your implementation this will vary
 * settings_attach(settings_ctx, loop);
 * \endcode
 * For settings owners, a setting is registered as follows:
 * \code{.c}
 * settings_register(settings_ctx, "sample_process", "sample_setting",
                     &sample_setting_data, sizeof(sample_setting_data),
                     SETTINGS_TYPE_BOOL,
                     optional_notify_callback, optional_callback_data);
 * \endcode
 * For a process that is tracking a non-owned setting, the process is similar:
 * \code{.c}
 * settings_add_watch(settings_ctx, "sample_process", "sample_setting",
                      &sample_setting_data, sizeof(sample_setting_data),
                      SETTINGS_TYPE_BOOL,
                      optional_notify_callback, optional_callback_data);
 * \endcode
 * The main difference here is that an owned setting will response to write
 * requests only, while a watch-only setting is updated on write responses
 * to stay in sync with successful updates as reported by settings owners.
 * @version v1.4
 * @date 2018-02-23
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libsbp/sbp.h>
#include <libsbp/settings.h>

#include <libsettings/settings.h>
#include <libsettings/settings_util.h>

#include <internal/request_state.h>
#include <internal/setting_data.h>
#include <internal/setting_type.h>
#include <internal/setting_type_enum.h>
#include <internal/setting_type_float.h>
#include <internal/setting_type_int.h>
#include <internal/setting_type_str.h>

#define REGISTER_TIMEOUT_MS 500
#define REGISTER_TRIES 5

#define WATCH_INIT_TIMEOUT_MS 500
#define WATCH_INIT_TRIES 5

static int log_err = 3;
static int log_warning = 4;

/**
 * @brief Settings Context
 *
 * This is the main context for managing client interactions with
 * the settings manager. It implements the client messaging context
 * as well as the list of types and settings necessary to perform
 * the registration, watching and callback functionality of the client.
 */
typedef struct settings_s {
  type_data_t *type_data_list;
  setting_data_t *setting_data_list;
  request_state_t request_state;
  bool register_resp_cb_registered;
  bool write_cb_registered;
  bool write_resp_cb_registered;
  bool read_resp_cb_registered;
  bool read_by_idx_resp_cb_registered;
  bool read_by_idx_done_cb_registered;
  sbp_msg_callbacks_node_t *register_resp_cb_node;
  sbp_msg_callbacks_node_t *write_cb_node;
  sbp_msg_callbacks_node_t *write_resp_cb_node;
  sbp_msg_callbacks_node_t *read_resp_cb_node;
  sbp_msg_callbacks_node_t *read_by_idx_resp_cb_node;
  sbp_msg_callbacks_node_t *read_by_idx_done_cb_node;
  settings_api_t client_iface;
  uint16_t sender_id;
  /* TODO: make independent structure of these */
  char resp_section[SETTINGS_BUFLEN];
  char resp_name[SETTINGS_BUFLEN];
  char resp_value[SETTINGS_BUFLEN];
  char resp_type[SETTINGS_BUFLEN];
  bool read_by_idx_done;
  settings_write_res_t status;
} settings_t;

static const char *const bool_enum_names[] = {"False", "True", NULL};

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

static void settings_update_value(settings_t *ctx, uint8_t *msg, uint8_t len)
{
  /* Extract parameters from message:
   * 4 null terminated strings: section, name, value and type.
   * Expect to find at least section, name and value.
   */
  const char *section = NULL, *name = NULL, *value = NULL, *type = NULL;
  if (settings_parse((char *)msg, len, &section, &name, &value, &type) < SETTINGS_TOKENS_VALUE) {
    ctx->client_iface.log(log_warning, "settings register resp cb, error parsing setting");
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

/**
 * @brief settings_register_resp_callback - callback for SBP_MSG_SETTINGS_REGISTER_RESP
 */
static void settings_register_resp_callback(uint16_t sender_id, uint8_t len, uint8_t *msg, void *context)
{
  settings_t *ctx = (settings_t *)context;
  (void)sender_id;

  if (sender_id != SBP_SENDER_ID) {
    ctx->client_iface.log(log_warning, "invalid sender %d != %d", sender_id, SBP_SENDER_ID);
    return;
  }

  /* Check for a response to a pending registration request */
  int state = request_state_check(&ctx->request_state, &ctx->client_iface, (char *)msg, len);

  if (state < 0) {
    ctx->client_iface.log(log_warning, "settings register resp cb, register req/resp mismatch");
    return;
  }
  
  if (state > 0) {
    ctx->client_iface.log(log_warning, "settings register resp cb, no pending registration request");
    return;
  }

  settings_update_value(ctx, msg, len);
}

/**
 * @brief settings_write_callback - callback for SBP_MSG_SETTINGS_WRITE
 */
static void settings_write_callback(uint16_t sender_id, uint8_t len, uint8_t *msg, void *context)
{
  settings_t *ctx = (settings_t *)context;
  (void)sender_id;

  if (sender_id != SBP_SENDER_ID) {
    ctx->client_iface.log(log_warning, "invalid sender %d != %d", sender_id, SBP_SENDER_ID);
    return;
  }

  settings_update_value(ctx, msg, len);
}

static int settings_update_watch_only(settings_t *ctx, const char *msg, uint8_t len)
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

/**
 * @brief settings_read_resp_callback - callback for SBP_MSG_SETTINGS_READ_RESP
 */
static void settings_read_resp_callback(uint16_t sender_id,
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

  if (settings_update_watch_only(ctx, read_response->setting, len) != 0) {
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

/**
 * @brief settings_write_resp_callback - callback for
 * SBP_MSG_SETTINGS_WRITE_RESP
 */
static void settings_write_resp_callback(uint16_t sender_id,
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
    /* Enable this warning back after ESD-1022 is fixed
     * ctx->client_iface.log(log_warning,
     *                       "setting write rejected (code: %d), not updating watched values",
     *                        write_response->status);
     */
    return;
  }

  if (settings_update_watch_only(ctx, write_response->setting, len - sizeof(write_response->status))
      != 0) {
    ctx->client_iface.log(log_warning, "error in settings read response message");
  }
}

/**
 * @brief settings_read_by_idx_resp_callback - callback for
 * SBP_MSG_SETTINGS_READ_BY_IDX_RESP
 */
static void settings_read_by_idx_resp_callback(uint16_t sender_id,
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

/**
 * @brief settings_read_by_idx_done_callback - callback for
 * SBP_MSG_SETTINGS_READ_BY_IDX_DONE
 */
static void settings_read_by_idx_done_callback(uint16_t sender_id,
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

/**
 * @brief settings_register_register_resp_callback - register callback for
 * SBP_MSG_SETTINGS_REGISTER_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_register_resp_callback(settings_t *ctx)
{
  if (ctx->register_resp_cb_registered) {
    /* Already done */
    return 0;
  }

  if (ctx->client_iface.register_cb(ctx->client_iface.ctx,
                                    SBP_MSG_SETTINGS_REGISTER_RESP,
                                    settings_register_resp_callback,
                                    ctx,
                                    &ctx->register_resp_cb_node)
      != 0) {
    ctx->client_iface.log(log_err, "error registering settings register resp callback");
    return -1;
  }

  ctx->register_resp_cb_registered = true;
  return 0;
}

/**
 * @brief settings_unregister_register_resp_callback - deregister callback for
 * SBP_MSG_SETTINGS_REGISTER_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if deregistration failed
 */
static int settings_unregister_register_resp_callback(settings_t *ctx)
{
  if (!ctx->register_resp_cb_registered) {
    return 0;
  }

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx, &ctx->register_resp_cb_node) != 0) {
    ctx->client_iface.log(log_err, "error unregistering settings write callback");
    return -1;
  }

  ctx->register_resp_cb_registered = false;
  return 0;
}

/**
 * @brief settings_register_write_callback - register callback for
 * SBP_MSG_SETTINGS_WRITE
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_write_callback(settings_t *ctx)
{
  if (ctx->write_cb_registered) {
    /* Already done */
    return 0;
  }

  if (ctx->client_iface.register_cb(ctx->client_iface.ctx,
                                    SBP_MSG_SETTINGS_WRITE,
                                    settings_write_callback,
                                    ctx,
                                    &ctx->write_cb_node)
      != 0) {
    ctx->client_iface.log(log_err, "error registering settings write callback");
    return -1;
  }

  ctx->write_cb_registered = true;
  return 0;
}

/**
 * @brief settings_unregister_write_callback - deregister callback for
 * SBP_MSG_SETTINGS_WRITE
 * @param ctx: settings context
 * @return zero on success, -1 if deregistration failed
 */
static int settings_unregister_write_callback(settings_t *ctx)
{
  if (!ctx->write_cb_registered) {
    return 0;
  }

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx, &ctx->write_cb_node) != 0) {
    ctx->client_iface.log(log_err, "error unregistering settings write callback");
    return -1;
  }

  ctx->write_cb_registered = false;
  return 0;
}

/**
 * @brief settings_register_read_resp_callback - register callback for
 * SBP_MSG_SETTINGS_READ_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_read_resp_callback(settings_t *ctx)
{
  if (ctx->read_resp_cb_registered) {
    /* Already done */
    return 0;
  }

  if (ctx->client_iface.register_cb(ctx->client_iface.ctx,
                                    SBP_MSG_SETTINGS_READ_RESP,
                                    settings_read_resp_callback,
                                    ctx,
                                    &ctx->read_resp_cb_node)
      != 0) {
    ctx->client_iface.log(log_err, "error registering settings read resp callback");
    return -1;
  }

  ctx->read_resp_cb_registered = true;
  return 0;
}

/**
 * @brief settings_unregister_read_resp_callback - deregister callback for
 * SBP_MSG_SETTINGS_READ_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if deregistration failed
 */
static int settings_unregister_read_resp_callback(settings_t *ctx)
{
  if (!ctx->read_resp_cb_registered) {
    return 0;
  }

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx, &ctx->read_resp_cb_node) != 0) {
    ctx->client_iface.log(log_err, "error unregistering settings read resp callback");
    return -1;
  }

  ctx->read_resp_cb_registered = false;
  return 0;
}

/**
 * @brief settings_register_write_resp_callback - register callback for
 * SBP_MSG_SETTINGS_WRITE_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_write_resp_callback(settings_t *ctx)
{
  if (ctx->write_resp_cb_registered) {
    /* Already done */
    return 0;
  }

  if (ctx->client_iface.register_cb(ctx->client_iface.ctx,
                                    SBP_MSG_SETTINGS_WRITE_RESP,
                                    settings_write_resp_callback,
                                    ctx,
                                    &ctx->write_resp_cb_node)
      != 0) {
    ctx->client_iface.log(log_err, "error registering settings write resp callback");
    return -1;
  }

  ctx->write_resp_cb_registered = true;
  return 0;
}

/**
 * @brief settings_unregister_write_resp_callback - deregister callback for
 * SBP_MSG_SETTINGS_WRITE_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if deregistration failed
 */
static int settings_unregister_write_resp_callback(settings_t *ctx)
{
  if (!ctx->write_resp_cb_registered) {
    return 0;
  }

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx, &ctx->write_resp_cb_node) != 0) {
    ctx->client_iface.log(log_err, "error unregistering settings write resp callback");
    return -1;
  }

  ctx->write_resp_cb_registered = false;
  return 0;
}

/**
 * @brief settings_register_read_by_idx_resp_callback - register callback for
 * SBP_MSG_SETTINGS_READ_BY_INDEX_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_read_by_idx_resp_callback(settings_t *ctx)
{
  if (ctx->read_by_idx_resp_cb_registered) {
    /* Already done */
    return 0;
  }

  if (ctx->client_iface.register_cb(ctx->client_iface.ctx,
                                    SBP_MSG_SETTINGS_READ_BY_INDEX_RESP,
                                    settings_read_by_idx_resp_callback,
                                    ctx,
                                    &ctx->read_by_idx_resp_cb_node)
      != 0) {
    ctx->client_iface.log(log_err, "error registering settings read by idx resp callback");
    return -1;
  }

  ctx->read_by_idx_resp_cb_registered = true;
  return 0;
}

/**
 * @brief settings_unregister_read_by_idx_resp_callback - deregister callback for
 * SBP_MSG_SETTINGS_READ_BY_INDEX_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if deregistration failed
 */
static int settings_unregister_read_by_idx_resp_callback(settings_t *ctx)
{
  if (!ctx->read_by_idx_resp_cb_registered) {
    return 0;
  }

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx, &ctx->read_by_idx_resp_cb_node) != 0) {
    ctx->client_iface.log(log_err, "error unregistering settings read by idx resp callback");
    return -1;
  }

  ctx->read_by_idx_resp_cb_registered = false;
  return 0;
}

/**
 * @brief settings_register_read_by_idx_done_callback - register callback for
 * SBP_MSG_SETTINGS_READ_BY_INDEX_DONE
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_read_by_idx_done_callback(settings_t *ctx)
{
  if (ctx->read_by_idx_done_cb_registered) {
    /* Already done */
    return 0;
  }

  if (ctx->client_iface.register_cb(ctx->client_iface.ctx,
                                    SBP_MSG_SETTINGS_READ_BY_INDEX_DONE,
                                    settings_read_by_idx_done_callback,
                                    ctx,
                                    &ctx->read_by_idx_done_cb_node)
      != 0) {
    ctx->client_iface.log(log_err, "error registering settings read by idx done callback");
    return -1;
  }

  ctx->read_by_idx_done_cb_registered = true;
  return 0;
}

/**
 * @brief settings_unregister_read_by_idx_done_callback - deregister callback for
 * SBP_MSG_SETTINGS_READ_BY_INDEX_DONE
 * @param ctx: settings context
 * @return zero on success, -1 if deregistration failed
 */
static int settings_unregister_read_by_idx_done_callback(settings_t *ctx)
{
  if (!ctx->read_by_idx_done_cb_registered) {
    return 0;
  }

  if (ctx->client_iface.unregister_cb(ctx->client_iface.ctx, &ctx->read_by_idx_done_cb_node) != 0) {
    ctx->client_iface.log(log_err, "error unregistering settings read by idx done callback");
    return -1;
  }

  ctx->read_by_idx_done_cb_registered = false;
  return 0;
}

/**
 * @brief setting_perform_request_reply_from
 * Performs a synchronous req/reply transation for the provided
 * message using the compare structure to match the header in callbacks.
 * Uses an explicit sender_id to allow for settings interactions with manager.
 * @param ctx: settings context
 * @param message_type: sbp message to use when sending the message
 * @param message: sbp message payload
 * @param message_length: length of payload
 * @param header_length: length of the substring to match during compare
 * @param timeout_ms: timeout between retries
 * @param retries: number of times to retry the transaction
 * @param sender_id: sender_id to use for outgoing message
 * @return zero on success, -1 the transaction failed to complete
 */
static int setting_perform_request_reply_from(settings_t *ctx,
                                              uint16_t message_type,
                                              char *message,
                                              uint8_t message_length,
                                              uint8_t header_length,
                                              uint32_t timeout_ms,
                                              uint8_t retries,
                                              uint16_t sender_id)
{
  request_state_init(&ctx->request_state, message_type, message, header_length);

  uint8_t tries = 0;
  bool success = false;

  /* Prime semaphores etc if applicable */
  if (ctx->client_iface.wait_init) {
    ctx->client_iface.wait_init(ctx->client_iface.ctx);
  }

  do {
    ctx->client_iface.send_from(ctx->client_iface.ctx,
                                message_type,
                                message_length,
                                (uint8_t *)message,
                                sender_id);

    if (ctx->client_iface.wait(ctx->client_iface.ctx, timeout_ms)) {
      size_t len1 = strlen(message) + 1;
      ctx->client_iface.log(log_warning,
                            "Waiting reply for msg id %d with %s.%s timed out",
                            message_type,
                            message,
                            message + len1);
    } else {
      success = request_state_match(&ctx->request_state);
    }

  } while (!success && (++tries < retries));

  /* Defuse semaphores etc if applicable */
  if (ctx->client_iface.wait_deinit) {
    ctx->client_iface.wait_deinit(ctx->client_iface.ctx);
  }

  request_state_deinit(&ctx->request_state);

  if (!success) {
    ctx->client_iface.log(log_warning,
                          "setting req/reply failed after %d retries (msg id: %d)",
                          tries,
                          message_type);
    return -1;
  }

  return 0;
}

/**
 * @brief setting_perform_request_reply - same as above but with implicit
 * sender_id
 * @param ctx: settings context
 * @param message_type: sbp message to use when sending the message
 * @param message: sbp message payload
 * @param message_length: length of payload
 * @param header_length: length of the substring to match during compare
 * @param timeout_ms: timeout between retries
 * @param retries: number of times to retry the transaction
 * @return zero on success, -1 the transaction failed to complete
 */
static int setting_perform_request_reply(settings_t *ctx,
                                         uint16_t message_type,
                                         char *message,
                                         uint8_t message_length,
                                         uint8_t header_length,
                                         uint16_t timeout_ms,
                                         uint8_t retries)
{
  return setting_perform_request_reply_from(ctx,
                                            message_type,
                                            message,
                                            message_length,
                                            header_length,
                                            timeout_ms,
                                            retries,
                                            ctx->sender_id);
}

/**
 * @brief setting_register - perform SBP_MSG_SETTINGS_REGISTER req/reply
 * @param ctx: settings context
 * @param setting_data: setting to register with settings daemon
 * @return zero on success, -1 the transaction failed to complete
 */
static int setting_register(settings_t *ctx, setting_data_t *setting_data)
{
  char msg[SETTINGS_BUFLEN] = {0};
  uint8_t msg_header_len;

  int msg_len = setting_data_format(setting_data, true, msg, sizeof(msg), &msg_header_len);

  if (msg_len < 0) {
    ctx->client_iface.log(log_err, "setting register message format failed");
    return -1;
  }

  return setting_perform_request_reply(ctx,
                                       SBP_MSG_SETTINGS_REGISTER,
                                       msg,
                                       msg_len,
                                       msg_header_len,
                                       REGISTER_TIMEOUT_MS,
                                       REGISTER_TRIES);
}

/**
 * @brief setting_read_watched_value - perform SBP_MSG_SETTINGS_READ_REQ
 * req/reply
 * @param ctx: setting context
 * @param setting_data: setting to read from settings daemon
 * @return zero on success, -1 the transaction failed to complete
 */
static int setting_read_watched_value(settings_t *ctx, setting_data_t *setting_data)
{
  int result = 0;
  /* Build message */
  char msg[SETTINGS_BUFLEN] = {0};
  uint8_t msg_len = 0;
  int l;

  if (!setting_data->watchonly) {
    ctx->client_iface.log(log_err, "cannot update non-watchonly setting manually");
    return -1;
  }

  l = settings_format(setting_data->section, setting_data->name, NULL, NULL, msg, sizeof(msg));
  if (l < 0) {
    ctx->client_iface.log(log_err, "error building settings read req message");
    return -1;
  }
  msg_len += l;

  if (settings_register_read_resp_callback(ctx) != 0) {
    ctx->client_iface.log(log_err, "error registering settings read callback");
    return -1;
  }

  result = setting_perform_request_reply_from(ctx,
                                              SBP_MSG_SETTINGS_READ_REQ,
                                              msg,
                                              msg_len,
                                              msg_len,
                                              WATCH_INIT_TIMEOUT_MS,
                                              WATCH_INIT_TRIES,
                                              SBP_SENDER_ID);

  settings_unregister_read_resp_callback(ctx);
  return result;
}

int settings_register_enum(settings_t *ctx, const char *const enum_names[], settings_type_t *type)
{
  assert(ctx != NULL);
  assert(enum_names != NULL);
  assert(type != NULL);

  return type_register(&ctx->type_data_list,
                       enum_to_string,
                       enum_from_string,
                       enum_format_type,
                       enum_names,
                       type);
}

/**
 * @brief settings_add_setting - internal subroutine for handling new settings
 * This method forwards all parameters to the setting factory to create a new
 * settings but also performs the addition of the setting to the settings
 * context internal list and performs either registration of the setting (if
 * owned) or a value update (if watch only) to fully initialize the new setting.
 * @param ctx: settings context
 * @param section: section identifier
 * @param name: setting name
 * @param var: non-owning reference to location the data is stored
 * @param var_len: length of data storage
 * @param type: type identifier
 * @param notify: optional notification callback
 * @param notify_context: optional data reference to pass during notification
 * @param readonly: set to true to disable value updates
 * @param watchonly: set to true to indicate a non-owned setting watch
 * @return zero on success, -1 if the addition of the setting has failed
 */
static int settings_add_setting(settings_t *ctx,
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
  assert(ctx != NULL);
  assert(section != NULL);
  assert(name != NULL);
  assert(var != NULL);

  if (setting_data_lookup(ctx->setting_data_list, section, name) != NULL) {
    ctx->client_iface.log(log_err, "setting add failed - duplicate setting");
    return -1;
  }

  setting_data_t *setting_data = setting_data_create(ctx->type_data_list,
                                                     section,
                                                     name,
                                                     var,
                                                     var_len,
                                                     type,
                                                     notify,
                                                     notify_context,
                                                     readonly,
                                                     watchonly);
  if (setting_data == NULL) {
    ctx->client_iface.log(log_err, "error creating setting data");
    return -1;
  }

  /* Add to list */
  setting_data_append(&ctx->setting_data_list, setting_data);

  if (watchonly) {
    if (settings_register_write_resp_callback(ctx) != 0) {
      ctx->client_iface.log(log_err, "error registering settings write resp callback");
    }
    if (setting_read_watched_value(ctx, setting_data) != 0) {
      ctx->client_iface.log(log_warning,
                            "Unable to read watched setting to initial value (%s.%s)",
                            section,
                            name);
    }
  } else {
    if (settings_register_register_resp_callback(ctx) != 0) {
      ctx->client_iface.log(log_err, "error registering settings register resp callback");
    }
    if (settings_register_write_callback(ctx) != 0) {
      ctx->client_iface.log(log_err, "error registering settings write callback");
    }
    if (setting_register(ctx, setting_data) != 0) {
      ctx->client_iface.log(log_err,
                            "error registering %s.%s with settings manager",
                            section,
                            name);
      setting_data_remove(ctx->setting_data_list, &setting_data);
      return -1;
    }
  }
  return 0;
}

int settings_register_setting(settings_t *ctx,
                              const char *section,
                              const char *name,
                              void *var,
                              size_t var_len,
                              settings_type_t type,
                              settings_notify_fn notify,
                              void *notify_context)
{
  return settings_add_setting(ctx,
                              section,
                              name,
                              var,
                              var_len,
                              type,
                              notify,
                              notify_context,
                              false,
                              false);
}

int settings_register_readonly(settings_t *ctx,
                               const char *section,
                               const char *name,
                               const void *var,
                               size_t var_len,
                               settings_type_t type)
{
  return settings_add_setting(ctx,
                              section,
                              name,
                              (void *)var,
                              var_len,
                              type,
                              NULL,
                              NULL,
                              true,
                              false);
}

int settings_register_watch(settings_t *ctx,
                            const char *section,
                            const char *name,
                            void *var,
                            size_t var_len,
                            settings_type_t type,
                            settings_notify_fn notify,
                            void *notify_context)
{
  return settings_add_setting(ctx,
                              section,
                              name,
                              var,
                              var_len,
                              type,
                              notify,
                              notify_context,
                              false,
                              true);
}

settings_write_res_t settings_write(settings_t *ctx,
                                    const char *section,
                                    const char *name,
                                    const void *value,
                                    size_t value_len,
                                    settings_type_t type)
{
  char msg[SETTINGS_BUFLEN] = {0};
  uint8_t msg_header_len;

  if (settings_register_write_resp_callback(ctx) != 0) {
    ctx->client_iface.log(log_err, "error registering settings write response callback");
  }

  setting_data_t *setting_data = setting_data_create(ctx->type_data_list,
                                                     section,
                                                     name,
                                                     (void *)value,
                                                     value_len,
                                                     type,
                                                     NULL,
                                                     NULL,
                                                     false,
                                                     false);
  if (setting_data == NULL) {
    ctx->client_iface.log(log_err, "settings write error while creating setting data");
    return -1;
  }

  int msg_len =
    setting_data_format(setting_data, false, msg, sizeof(msg), &msg_header_len);

  if (msg_len < 0) {
    ctx->client_iface.log(log_err, "setting write error format failed");
    setting_data_destroy(setting_data);
    return -1;
  }

  /* This will be updated in the settings_write_resp_callback */
  ctx->status = SETTINGS_WR_TIMEOUT;

  setting_perform_request_reply_from(ctx,
                                     SBP_MSG_SETTINGS_WRITE,
                                     msg,
                                     msg_len,
                                     msg_header_len,
                                     REGISTER_TIMEOUT_MS,
                                     REGISTER_TRIES,
                                     SBP_SENDER_ID);

  setting_data_destroy(setting_data);

  return ctx->status;
}

settings_write_res_t settings_write_int(settings_t *ctx,
                                        const char *section,
                                        const char *name,
                                        int value)
{
  return settings_write(ctx, section, name, &value, sizeof(value), SETTINGS_TYPE_INT);
}

settings_write_res_t settings_write_float(settings_t *ctx,
                                          const char *section,
                                          const char *name,
                                          float value)
{
  return settings_write(ctx, section, name, &value, sizeof(value), SETTINGS_TYPE_FLOAT);
}

settings_write_res_t settings_write_str(settings_t *ctx,
                                        const char *section,
                                        const char *name,
                                        const char *str)
{
  return settings_write(ctx, section, name, str, strlen(str), SETTINGS_TYPE_STRING);
}

settings_write_res_t settings_write_bool(settings_t *ctx,
                                         const char *section,
                                         const char *name,
                                         bool value)
{
  return settings_write(ctx, section, name, &value, sizeof(value), SETTINGS_TYPE_BOOL);
}

int settings_read(settings_t *ctx,
                  const char *section,
                  const char *name,
                  void *value,
                  size_t value_len,
                  settings_type_t type)
{
  assert(ctx);
  assert(section);
  assert(name);
  assert(value);
  assert(value_len);

  /* Build message */
  char msg[SETTINGS_BUFLEN] = {0};
  int msg_len = settings_format(section, name, NULL, NULL, msg, sizeof(msg));

  if (msg_len < 0) {
    ctx->client_iface.log(log_err, "error building settings read req message");
    return -1;
  }

  if (settings_register_read_resp_callback(ctx) != 0) {
    ctx->client_iface.log(log_err, "error registering settings read resp callback");
    return -1;
  }

  ctx->resp_section[0] = '\0';
  ctx->resp_name[0] = '\0';
  ctx->resp_value[0] = '\0';
  ctx->resp_type[0] = '\0';

  int res = setting_perform_request_reply_from(ctx,
                                               SBP_MSG_SETTINGS_READ_REQ,
                                               msg,
                                               msg_len,
                                               msg_len,
                                               WATCH_INIT_TIMEOUT_MS,
                                               WATCH_INIT_TRIES,
                                               SBP_SENDER_ID);

  settings_unregister_read_resp_callback(ctx);

  if (res != 0) {
    return res;
  }

  settings_type_t parsed_type = SETTINGS_TYPE_STRING;

  if (strlen(ctx->resp_type) != 0) {
    const char *cmp = "enum:";
    if (strncmp(ctx->resp_type, cmp, strlen(cmp)) != 0) {
      parsed_type = strtol(ctx->resp_type, NULL, 10);
    }
  } else {
    parsed_type = type;
  }

  if (type != parsed_type) {
    ctx->client_iface.log(log_err, "setting types don't match");
    return -1;
  }

  const type_data_t *td = type_data_lookup(ctx->type_data_list, parsed_type);

  if (td == NULL) {
    ctx->client_iface.log(log_err, "unknown setting type");
    return -1;
  }

  if (!td->from_string(td->priv, value, value_len, ctx->resp_value)) {
    ctx->client_iface.log(log_err, "value parsing failed");
    return -1;
  }

  return 0;
}

int settings_read_int(settings_t *ctx, const char *section, const char *name, int *value)
{
  return settings_read(ctx, section, name, value, sizeof(int), SETTINGS_TYPE_INT);
}

int settings_read_float(settings_t *ctx, const char *section, const char *name, float *value)
{
  return settings_read(ctx, section, name, value, sizeof(float), SETTINGS_TYPE_FLOAT);
}

int settings_read_str(settings_t *ctx,
                      const char *section,
                      const char *name,
                      char *str,
                      size_t str_len)
{
  return settings_read(ctx, section, name, str, str_len, SETTINGS_TYPE_STRING);
}

int settings_read_bool(settings_t *ctx, const char *section, const char *name, bool *value)
{
  return settings_read(ctx, section, name, value, sizeof(bool), SETTINGS_TYPE_BOOL);
}

int settings_read_by_idx(settings_t *ctx,
                         uint16_t idx,
                         char *section,
                         size_t section_len,
                         char *name,
                         size_t name_len,
                         char *value,
                         size_t value_len,
                         char *type,
                         size_t type_len)
{
  assert(section_len > 0);
  assert(name_len > 0);
  assert(value_len > 0);
  assert(type_len > 0);

  int res = -1;

  if (settings_register_read_by_idx_resp_callback(ctx) != 0) {
    ctx->client_iface.log(log_err, "error registering settings read by idx resp callback");
    goto read_by_idx_cleanup;
  }

  if (settings_register_read_by_idx_done_callback(ctx) != 0) {
    ctx->client_iface.log(log_err, "error registering settings read by idx done callback");
    goto read_by_idx_cleanup;
  }

  ctx->resp_section[0] = '\0';
  ctx->resp_name[0] = '\0';
  ctx->resp_value[0] = '\0';
  ctx->resp_type[0] = '\0';
  ctx->read_by_idx_done = false;

  res = setting_perform_request_reply_from(ctx,
                                           SBP_MSG_SETTINGS_READ_BY_INDEX_REQ,
                                           (char *)&idx,
                                           sizeof(uint16_t),
                                           sizeof(uint16_t),
                                           WATCH_INIT_TIMEOUT_MS,
                                           WATCH_INIT_TRIES,
                                           SBP_SENDER_ID);

  if (res != 0) {
    ctx->client_iface.log(log_err, "read by idx request failed");
    goto read_by_idx_cleanup;
  }

  strncpy(section, ctx->resp_section, section_len);
  strncpy(name, ctx->resp_name, name_len);
  strncpy(value, ctx->resp_value, value_len);
  strncpy(type, ctx->resp_type, type_len);

read_by_idx_cleanup:
  settings_unregister_read_by_idx_resp_callback(ctx);
  settings_unregister_read_by_idx_done_callback(ctx);

  /* Error */
  if (res != 0) {
    return res;
  }

  /* Last index was read */
  if (ctx->read_by_idx_done) {
    return 1;
  }

  /* No errors, next index to be read */
  return 0;
}

settings_t *settings_create(uint16_t sender_id, settings_api_t *client_iface)
{
  assert(client_iface != NULL);
  assert(client_iface->log != NULL);

  settings_t *ctx = (settings_t *)malloc(sizeof(*ctx));
  if (ctx == NULL) {
    client_iface->log(log_err, "error allocating context");
    return ctx;
  }

  ctx->sender_id = sender_id;

  ctx->client_iface = *client_iface;

  ctx->type_data_list = NULL;
  ctx->setting_data_list = NULL;

  ctx->request_state.pending = false;

  ctx->write_cb_registered = false;
  ctx->write_resp_cb_registered = false;
  ctx->read_resp_cb_registered = false;
  ctx->read_by_idx_resp_cb_registered = false;
  ctx->read_by_idx_done_cb_registered = false;

  /* Register standard types */
  settings_type_t type;

  int ret = type_register(&ctx->type_data_list, int_to_string, int_from_string, NULL, NULL, &type);
  /* To make cythonizer happy.. */
  (void)ret;

  assert(ret == 0);
  assert(type == SETTINGS_TYPE_INT);

  ret = type_register(&ctx->type_data_list, float_to_string, float_from_string, NULL, NULL, &type);
  assert(ret == 0);
  assert(type == SETTINGS_TYPE_FLOAT);

  ret = type_register(&ctx->type_data_list, str_to_string, str_from_string, NULL, NULL, &type);
  assert(ret == 0);
  assert(type == SETTINGS_TYPE_STRING);

  ret = type_register(&ctx->type_data_list,
                      enum_to_string,
                      enum_from_string,
                      enum_format_type,
                      bool_enum_names,
                      &type);
  assert(ret == 0);
  assert(type == SETTINGS_TYPE_BOOL);

  return ctx;
}

/**
 * @brief members_destroy - deinit for settings context members
 * @param ctx: settings context to deinit
 */
static void members_destroy(settings_t *ctx)
{
  type_data_free(ctx->type_data_list);
  setting_data_free(ctx->setting_data_list);
}

void settings_destroy(settings_t **ctx)
{
  assert(ctx != NULL);
  assert(*ctx != NULL);
  (*ctx)->client_iface.log(log_err, "Releasing settings framework");
  settings_unregister_register_resp_callback(*ctx);
  settings_unregister_write_callback(*ctx);
  settings_unregister_write_resp_callback(*ctx);
  settings_unregister_read_resp_callback(*ctx);
  settings_unregister_read_by_idx_resp_callback(*ctx);
  settings_unregister_read_by_idx_done_callback(*ctx);

  members_destroy(*ctx);
  free(*ctx);
  *ctx = NULL;
}
