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
#include <internal/setting_def.h>
#include <internal/setting_sbp_cb.h>
#include <internal/setting_type.h>
#include <internal/setting_type_enum.h>
#include <internal/setting_type_float.h>
#include <internal/setting_type_int.h>
#include <internal/setting_type_str.h>

#define REGISTER_TIMEOUT_MS 500
#define REGISTER_TRIES 5

#define WATCH_INIT_TIMEOUT_MS 500
#define WATCH_INIT_TRIES 5

static const char *const bool_enum_names[] = {"False", "True", NULL};

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

  if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_READ_RESP) < 0) {
    ctx->client_iface.log(log_err, "error registering settings read resp callback");
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

  setting_sbp_cb_unregister(ctx, SBP_MSG_SETTINGS_READ_RESP);
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
    if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_WRITE_RESP) < 0) {
      ctx->client_iface.log(log_err, "error registering settings write resp callback");
    }
    if (setting_read_watched_value(ctx, setting_data) != 0) {
      ctx->client_iface.log(log_warning,
                            "Unable to read watched setting to initial value (%s.%s)",
                            section,
                            name);
    }
  } else {
    if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_REGISTER_RESP) < 0) {
      ctx->client_iface.log(log_err, "error registering settings register resp callback");
    }
    if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_WRITE) < 0) {
      ctx->client_iface.log(log_err, "error registering settings write callback");
    }
    if (setting_register(ctx, setting_data) != 0) {
      ctx->client_iface.log(log_err,
                            "error registering %s.%s with settings manager",
                            section,
                            name);
      setting_data_remove(&ctx->setting_data_list, &setting_data);
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

  if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_WRITE_RESP) < 0) {
    ctx->client_iface.log(log_err, "error registering settings write response callback");
    return -1;
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

  int msg_len = setting_data_format(setting_data, false, msg, sizeof(msg), &msg_header_len);

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

  if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_READ_RESP) < 0) {
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

  setting_sbp_cb_unregister(ctx, SBP_MSG_SETTINGS_READ_RESP);

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

  if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_READ_BY_INDEX_RESP) < 0) {
    ctx->client_iface.log(log_err, "error registering settings read by idx resp callback");
    goto read_by_idx_cleanup;
  }

  if (setting_sbp_cb_register(ctx, SBP_MSG_SETTINGS_READ_BY_INDEX_DONE) < 0) {
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
  setting_sbp_cb_unregister(ctx, SBP_MSG_SETTINGS_READ_BY_INDEX_RESP);
  setting_sbp_cb_unregister(ctx, SBP_MSG_SETTINGS_READ_BY_INDEX_DONE);

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
  ctx->sbp_cb_list = NULL;

  ctx->request_state.pending = false;

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
  setting_sbp_cb_unregister(*ctx, SBP_MSG_SETTINGS_REGISTER_RESP);
  setting_sbp_cb_unregister(*ctx, SBP_MSG_SETTINGS_WRITE);
  setting_sbp_cb_unregister(*ctx, SBP_MSG_SETTINGS_WRITE_RESP);
  setting_sbp_cb_unregister(*ctx, SBP_MSG_SETTINGS_READ_RESP);
  setting_sbp_cb_unregister(*ctx, SBP_MSG_SETTINGS_READ_BY_INDEX_RESP);
  setting_sbp_cb_unregister(*ctx, SBP_MSG_SETTINGS_READ_BY_INDEX_DONE);

  members_destroy(*ctx);
  free(*ctx);
  *ctx = NULL;
}
