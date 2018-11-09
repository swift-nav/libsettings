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
 * @file settings_register.c
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
#include <unistd.h>

#include <libsbp/sbp.h>
#include <libsbp/settings.h>

#include <libsettings/settings_register.h>

#define REGISTER_TIMEOUT_MS 500
#define REGISTER_TRIES 5

#define WATCH_INIT_TIMEOUT_MS 500
#define WATCH_INIT_TRIES 5

#define SBP_PAYLOAD_SIZE_MAX 255

static uint16_t sys_sender_id = -1;

void settings_sender_id_set(uint16_t id)
{
  sys_sender_id = id;
}

static int log_err = 3;
static int log_warning = 4;

typedef int (*to_string_fn)(const void *priv, char *str, int slen, const void *blob, int blen);
typedef bool (*from_string_fn)(const void *priv, void *blob, int blen, const char *str);
typedef int (*format_type_fn)(const void *priv, char *str, int slen);

/**
 * @brief Type Data
 *
 * This structure encapsulates the codec for values of a given type
 * which the settings context uses to build a list of known types
 * that it can support when settings are added to the settings list.
 */
typedef struct type_data_s {
  to_string_fn to_string;
  from_string_fn from_string;
  format_type_fn format_type;
  const void *priv;
  struct type_data_s *next;
} type_data_t;

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

/**
 * @brief Settings Context
 *
 * This is the main context for managing client interactions with
 * the settings manager. It implements the client messaging context
 * as well as the list of types and settings necessary to perform
 * the registration, watching and callback functionality of the client.
 */
struct settings_s {
  type_data_t *type_data_list;
  setting_data_t *setting_data_list;
  registration_state_t registration_state;
  bool write_callback_registered;
  bool write_resp_callback_registered;
  bool read_resp_callback_registered;
  sbp_msg_callbacks_node_t *write_cb_node;
  sbp_msg_callbacks_node_t *write_resp_cb_node;
  sbp_msg_callbacks_node_t *read_resp_cb_node;
};

static const char *const bool_enum_names[] = {"False", "True", NULL};

static settings_api_t settings_api = {
  .ctx = NULL,
  .send = NULL,
  .send_from = NULL,
  .wait_init = NULL,
  .wait = NULL,
  .wait_deinit = NULL,
  .signal = NULL,
  .register_cb = NULL,
  .unregister_cb = NULL,
};

void settings_api_init(const settings_api_t *api)
{
  settings_api = *api;
}

/**
 * @brief setting_send_write_response
 * @param write_response: pre-formatted read response sbp message
 * @param len: length of the message
 * @return zero on success, -1 if message failed to send
 */
static int setting_send_write_response(msg_settings_write_resp_t *write_response, uint8_t len)
{
  if (settings_api.send(settings_api.ctx,
                        SBP_MSG_SETTINGS_WRITE_RESP,
                        len,
                        (uint8_t *)write_response)
      != 0) {
    settings_api.log(log_err, "sending settings write response failed");
    return -1;
  }
  return 0;
}

/**
 * @brief compare_init - set up compare structure for synchronous req/reply
 * @param ctx: settings context
 * @param data: formatted settings header string to match with incoming messages
 * @param data_len: length of match string
 */
static void compare_init(settings_t *ctx, const char *data, size_t data_len)
{
  registration_state_t *r = &ctx->registration_state;
  memset(r, 0, sizeof(registration_state_t));

  assert(data_len <= sizeof(r->compare_data));

  memcpy(r->compare_data, data, data_len);
  r->compare_data_len = data_len;
  r->match = false;
  r->pending = true;
}

/**
 * @brief compare_check - used by message callbacks to perform comparison
 * @param ctx: settings context
 * @param data: settings message payload string to match with header string
 * @param data_len: length of payload string
 * @return 0 for match, 1 no comparison pending, -1 for comparison failure
 */
static int compare_check(settings_t *ctx, const char *data, size_t data_len)
{
  assert(ctx);
  assert(data);

  registration_state_t *r = &ctx->registration_state;

  assert(r);

  if (!r->pending) {
    return 1;
  }

  if ((data_len >= r->compare_data_len)
      && (memcmp(data, r->compare_data, r->compare_data_len) == 0)) {
    r->match = true;
    r->pending = false;
    settings_api.signal(settings_api.ctx);
    return 0;
  }

  return -1;
}

/**
 * @brief compare_deinit - clean up compare structure after transaction
 * @param ctx: settings context
 */
static void compare_deinit(settings_t *ctx)
{
  registration_state_t *r = &ctx->registration_state;
  r->pending = false;
}

/**
 * @brief setting_update_value - process value string and update internal data
 * on success
 * @param setting_data: setting to update
 * @param value: value string to evaluate
 * @param write_result: result to pass to write response
 */
static void setting_update_value(setting_data_t *setting_data,
                                 const char *value,
                                 uint8_t *write_result)
{
  if (setting_data->readonly) {
    *write_result = SETTINGS_WR_READ_ONLY;
    return;
  }

  *write_result = SETTINGS_WR_OK;
  /* Store copy and update value */
  memcpy(setting_data->var_copy, setting_data->var, setting_data->var_len);
  if (!setting_data->type_data->from_string(setting_data->type_data->priv,
                                            setting_data->var,
                                            setting_data->var_len,
                                            value)) {
    /* Revert value if conversion fails */
    memcpy(setting_data->var, setting_data->var_copy, setting_data->var_len);
    *write_result = SETTINGS_WR_PARSE_FAILED;
    return;
  }

  if (NULL == setting_data->notify) {
    return;
  }

  /* Call notify function */
  int notify_response = setting_data->notify(setting_data->notify_context);

  if (setting_data->watchonly) {
    /* No need for actions */
    return;
  }

  if (notify_response != SETTINGS_WR_OK) {
    /* Revert value if notify returns error */
    memcpy(setting_data->var, setting_data->var_copy, setting_data->var_len);
    *write_result = notify_response;
  }
}

/**
 * @brief message_header_get - to allow formatting of identity only
 * @param setting_data: the setting to format
 * @param buf: buffer to hold formatted header string
 * @param blen: length of the destination buffer
 * @return bytes written to the buffer, -1 in case of failure
 */
static int message_header_get(setting_data_t *setting_data, char *buf, int blen)
{
  int n = 0;
  int l;

  /* Section */
  l = snprintf(&buf[n], blen - n, "%s", setting_data->section);
  if ((l < 0) || (l >= blen - n)) {
    return -1;
  }
  n += l + 1;

  /* Name */
  l = snprintf(&buf[n], blen - n, "%s", setting_data->name);
  if ((l < 0) || (l >= blen - n)) {
    return -1;
  }
  n += l + 1;

  return n;
}

/**
 * @brief message_data_get - formatting of value and type
 * @param setting_data: the setting to format
 * @param buf: buffer to hold formatted data string
 * @param blen: length of the destination buffer
 * @return bytes written to the buffer, -1 in case of failure
 */
static int message_data_get(setting_data_t *setting_data, char *buf, int blen)
{
  int n = 0;
  int l;

  /* Value */
  l = setting_data->type_data->to_string(setting_data->type_data->priv,
                                         &buf[n],
                                         blen - n,
                                         setting_data->var,
                                         setting_data->var_len);
  if ((l < 0) || (l >= blen - n)) {
    return -1;
  }
  n += l + 1;

  /* Type information */
  if (setting_data->type_data->format_type != NULL) {
    l = setting_data->type_data->format_type(setting_data->type_data->priv, &buf[n], blen - n);
    if ((l < 0) || (l >= blen - n)) {
      return -1;
    }
    n += l + 1;
  }

  return n;
}

/**
 * @brief setting_format_setting - formats a fully formed setting message
 * payload
 * @param setting_data: the setting to format
 * @param buf: buffer to hold formatted setting string
 * @param len: length of the destination buffer
 * @return bytes written to the buffer, -1 in case of failure
 */
static int setting_format_setting(setting_data_t *setting_data, char *buf, int len)
{
  int result = 0;
  int written = 0;

  result = message_header_get(setting_data, buf, len - written);
  if (result < 0) {
    return result;
  }
  written += result;

  result = message_data_get(setting_data, buf + written, len - written);
  if (result < 0) {
    return result;
  }
  written += result;

  return written;
}

/**
 * @brief setting_data_lookup - retrieves setting node from settings context
 * @param ctx: settings context
 * @param section: setting section string to match
 * @param name: setting name string to match
 * @return the setting type entry if a match is found, otherwise NULL
 */
static setting_data_t *setting_data_lookup(settings_t *ctx, const char *section, const char *name)
{
  setting_data_t *setting_data = ctx->setting_data_list;
  while (setting_data != NULL) {
    if ((strcmp(setting_data->section, section) == 0) && (strcmp(setting_data->name, name) == 0)) {
      break;
    }
    setting_data = setting_data->next;
  }
  return setting_data;
}

static int settings_parse(const char *msg,
                          uint8_t msg_n,
                          const char **section,
                          const char **name,
                          const char **value)
{
  const char **result_holders[] = {section, name, value};
  uint8_t start = 0;
  uint8_t end = 0;
  for (uint8_t i = 0; i < sizeof(result_holders) / sizeof(*result_holders); i++) {
    bool found = false;
    *(result_holders[i]) = NULL;
    while (end < msg_n) {
      if (msg[end] == '\0') {
        // don't allow empty strings before the third term
        if (end == start && i < 2) {
          return -1;
        } else {
          *(result_holders[i]) = (const char *)msg + start;
          start = (uint8_t)(end + 1);
          found = true;
        }
      }
      end++;
      if (found) {
        break;
      }
    }
  }
  return 0;
}

/**
 * @brief settings_write_callback - callback for SBP_MSG_SETTINGS_WRITE
 */
static void settings_write_callback(uint16_t sender_id, uint8_t len, uint8_t *msg, void *context)
{
  settings_t *ctx = (settings_t *)context;
  (void)sender_id;

  if (sender_id != SBP_SENDER_ID) {
    settings_api.log(log_warning, "invalid sender %d != %d", sender_id, SBP_SENDER_ID);
    return;
  }

  /* Check for a response to a pending registration request */
  compare_check(ctx, (char *)msg, len);

  /* Extract parameters from message:
   * 3 null terminated strings: section, setting and value
   */
  const char *section = NULL;
  const char *name = NULL;
  const char *value = NULL;
  if (settings_parse((char *)msg, len, &section, &name, &value) != 0) {
    settings_api.log(log_warning, "settings write message failed");
    return;
  }

  /* Look up setting data */
  setting_data_t *setting_data = setting_data_lookup(ctx, section, name);
  if (setting_data == NULL) {
    return;
  }

  if (setting_data->watchonly) {
    return;
  }

  uint8_t write_result = SETTINGS_WR_OK;
  setting_update_value(setting_data, value, &write_result);

  uint8_t resp[SBP_PAYLOAD_SIZE_MAX];
  uint8_t resp_len = 0;
  msg_settings_write_resp_t *write_response = (msg_settings_write_resp_t *)resp;
  write_response->status = write_result;
  resp_len += sizeof(write_response->status);
  int l =
    setting_format_setting(setting_data, write_response->setting, SBP_PAYLOAD_SIZE_MAX - resp_len);
  if (l < 0) {
    return;
  }
  resp_len += l;

  setting_send_write_response(write_response, resp_len);
}

static int settings_update_watch_only(settings_t *ctx, char *msg, uint8_t len)
{
  /* Extract parameters from message:
   * 3 null terminated strings: section, setting and value
   */
  const char *section = NULL;
  const char *name = NULL;
  const char *value = NULL;
  if (settings_parse(msg, len, &section, &name, &value) != 0) {
    settings_api.log(log_warning, "error parsing setting");
    return -1;
  }

  /* Look up setting data */
  setting_data_t *setting_data = setting_data_lookup(ctx, section, name);
  if (setting_data == NULL) {
    return 0;
  }

  if (!setting_data->watchonly) {
    return 0;
  }

  uint8_t write_result = SETTINGS_WR_OK;
  setting_update_value(setting_data, value, &write_result);
  if (write_result != SETTINGS_WR_OK) {
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
  msg_settings_read_resp_t *read_response = (msg_settings_read_resp_t *)msg;

  /* Check for a response to a pending request */
  compare_check(ctx, read_response->setting, len);

  if (settings_update_watch_only(ctx, read_response->setting, len) != 0) {
    settings_api.log(log_warning, "error in settings read response message");
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

  /* Check for a response to a pending request */
  compare_check(ctx, write_response->setting, len - sizeof(write_response->status));

  if (settings_update_watch_only(ctx, write_response->setting, len - sizeof(write_response->status))
      != 0) {
    settings_api.log(log_warning, "error in settings read response message");
  }
}

/**
 * @brief settings_register_write_callback - register callback for
 * SBP_MSG_SETTINGS_WRITE
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_write_callback(settings_t *ctx)
{
  if (ctx->write_callback_registered) {
    /* Already done */
    return 0;
  }

  if (settings_api.register_cb(settings_api.ctx,
                               SBP_MSG_SETTINGS_WRITE,
                               settings_write_callback,
                               ctx,
                               &ctx->write_cb_node)
      != 0) {
    settings_api.log(log_err, "error registering settings write callback");
    return -1;
  }

  ctx->write_callback_registered = true;
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
  if (ctx->read_resp_callback_registered) {
    /* Already done */
    return 0;
  }

  if (settings_api.register_cb(settings_api.ctx,
                               SBP_MSG_SETTINGS_READ_RESP,
                               settings_read_resp_callback,
                               ctx,
                               &ctx->read_resp_cb_node)
      != 0) {
    settings_api.log(log_err, "error registering settings read resp callback");
    return -1;
  }

  ctx->read_resp_callback_registered = true;
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
  if (!ctx->read_resp_callback_registered) {
    return 0;
  }

  if (settings_api.unregister_cb(settings_api.ctx, &ctx->read_resp_cb_node) != 0) {
    settings_api.log(log_err, "error unregistering settings read resp callback");
    return -1;
  }

  ctx->read_resp_callback_registered = false;
  return 0;
}

/**
 * @brief settings_register_write_resp_callback - register callback for
 * SBP_MSG_SETTINGS_READ_RESP
 * @param ctx: settings context
 * @return zero on success, -1 if registration failed
 */
static int settings_register_write_resp_callback(settings_t *ctx)
{
  if (ctx->write_resp_callback_registered) {
    /* Already done */
    return 0;
  }

  if (settings_api.register_cb(settings_api.ctx,
                               SBP_MSG_SETTINGS_WRITE_RESP,
                               settings_write_resp_callback,
                               ctx,
                               &ctx->write_resp_cb_node)
      != 0) {
    settings_api.log(log_err, "error registering settings write resp callback");
    return -1;
  }

  ctx->write_resp_callback_registered = true;
  return 0;
}

static int float_to_string(const void *priv, char *str, int slen, const void *blob, int blen)
{
  (void)priv;

  switch (blen) {
  case 4: return snprintf(str, slen, "%.12g", (double)*(float *)blob);
  case 8: return snprintf(str, slen, "%.12g", *(double *)blob);
  default: return -1;
  }
}

static bool float_from_string(const void *priv, void *blob, int blen, const char *str)
{
  (void)priv;

  switch (blen) {
  case 4: return sscanf(str, "%g", (float *)blob) == 1;
  case 8: return sscanf(str, "%lg", (double *)blob) == 1;
  default: return false;
  }
}

static int int_to_string(const void *priv, char *str, int slen, const void *blob, int blen)
{
  (void)priv;

  switch (blen) {
  case 1: return snprintf(str, slen, "%hhd", *(s8 *)blob);
  case 2: return snprintf(str, slen, "%hd", *(s16 *)blob);
  case 4: return snprintf(str, slen, "%" PRId32, *(s32 *)blob);
  default: return -1;
  }
}

static bool int_from_string(const void *priv, void *blob, int blen, const char *str)
{
  (void)priv;

  switch (blen) {
  case 1: {
    s16 tmp;
    /* Newlib's crappy sscanf doesn't understand %hhd */
    if (sscanf(str, "%hd", &tmp) == 1) {
      *(s8 *)blob = tmp;
      return true;
    }
    return false;
  }
  case 2: return sscanf(str, "%hd", (s16 *)blob) == 1;
  case 4: return sscanf(str, "%" PRId32, (s32 *)blob) == 1;
  default: return false;
  }
}

static int str_to_string(const void *priv, char *str, int slen, const void *blob, int blen)
{
  (void)priv;
  (void)blen;

  return snprintf(str, slen, "%s", (char *)blob);
}

static bool str_from_string(const void *priv, void *blob, int blen, const char *str)
{
  (void)priv;

  int l = snprintf(blob, blen, "%s", str);
  if ((l < 0) || (l >= blen)) {
    return false;
  }

  return true;
}

static int enum_to_string(const void *priv, char *str, int slen, const void *blob, int blen)
{
  (void)blen;

  const char *const *enum_names = priv;
  int index = *(uint8_t *)blob;
  return snprintf(str, slen, "%s", enum_names[index]);
}

static bool enum_from_string(const void *priv, void *blob, int blen, const char *str)
{
  (void)blen;

  const char *const *enum_names = priv;
  int i;

  for (i = 0; enum_names[i] && (strcmp(str, enum_names[i]) != 0); i++) {
    ;
  }

  if (!enum_names[i]) {
    return false;
  }

  *(uint8_t *)blob = i;

  return true;
}

static int enum_format_type(const void *priv, char *str, int slen)
{
  int n = 0;
  int l;

  /* Print "enum:" header */
  l = snprintf(&str[n], slen - n, "enum:");
  if (l < 0) {
    return l;
  }
  n += l;

  /* Print enum names separated by commas */
  for (const char *const *enum_names = priv; *enum_names; enum_names++) {
    if (n < slen) {
      l = snprintf(&str[n], slen - n, "%s,", *enum_names);
      if (l < 0) {
        return l;
      }
      n += l;
    } else {
      n += strlen(*enum_names) + 1;
    }
  }

  /* Replace last comma with NULL */
  if ((n > 0) && (n - 1 < slen)) {
    str[n - 1] = '\0';
    n--;
  }

  return n;
}

/**
 * @brief type_data_lookup - retrieves type node from settings context
 * @param ctx: settings context
 * @param type: type struct to be matched
 * @return the setting type entry if a match is found, otherwise NULL
 */
static type_data_t *type_data_lookup(settings_t *ctx, settings_type_t type)
{
  type_data_t *type_data = ctx->type_data_list;
  for (int i = 0; i < type && type_data != NULL; i++) {
    type_data = type_data->next;
  }
  return type_data;
}

static void setting_data_list_insert(settings_t *ctx, setting_data_t *setting_data)
{
  if (ctx->setting_data_list == NULL) {
    ctx->setting_data_list = setting_data;
  } else {
    setting_data_t *s;
    /* Find last element in the same section */
    for (s = ctx->setting_data_list; s->next != NULL; s = s->next) {
      if ((strcmp(s->section, setting_data->section) == 0)
          && (strcmp(s->next->section, setting_data->section) != 0)) {
        break;
      }
    }
    setting_data->next = s->next;
    s->next = setting_data;
  }
}

/**
 * @brief compare_match - returns status of current comparison
 * This is used as the value to block on until the comparison has been matched
 * successfully or until (based on implementation) a number of retries or a
 * timeout has expired.
 * @param ctx: settings context
 * @return true if response was matched, false if not response has been received
 */
static bool compare_match(settings_t *ctx)
{
  registration_state_t *r = &ctx->registration_state;
  return r->match;
}

/**
 * @brief type_register - register type data for reference when adding settings
 * @param ctx: settings context
 * @param to_string: serialization method
 * @param from_string: deserialization method
 * @param format_type: ?
 * @param priv: private data used in ser/des methods
 * @param type: type enum that is used to identify this type
 * @return
 */
static int type_register(settings_t *ctx,
                         to_string_fn to_string,
                         from_string_fn from_string,
                         format_type_fn format_type,
                         const void *priv,
                         settings_type_t *type)
{
  type_data_t *type_data = (type_data_t *)malloc(sizeof(*type_data));
  if (type_data == NULL) {
    settings_api.log(log_err, "error allocating type data");
    return -1;
  }

  *type_data = (type_data_t){.to_string = to_string,
                             .from_string = from_string,
                             .format_type = format_type,
                             .priv = priv,
                             .next = NULL};

  /* Add to list */
  settings_type_t next_type = 0;
  type_data_t **p_next = &ctx->type_data_list;
  while (*p_next != NULL) {
    p_next = &(*p_next)->next;
    next_type++;
  }

  *p_next = type_data;
  *type = next_type;
  return 0;
}

/**
 * @brief setting_data_members_destroy - deinit for settings data
 * @param setting_data: setting to deinit
 */
static void setting_data_members_destroy(setting_data_t *setting_data)
{
  if (setting_data->section) {
    free(setting_data->section);
    setting_data->section = NULL;
  }

  if (setting_data->name) {
    free(setting_data->name);
    setting_data->name = NULL;
  }

  if (setting_data->var_copy) {
    free(setting_data->var_copy);
    setting_data->var_copy = NULL;
  }
}

/**
 * @brief setting_data_list_remove - remove a setting from the settings context
 * @param ctx: settings context
 * @param setting_data: setting to remove
 */
static void setting_data_list_remove(settings_t *ctx, setting_data_t **setting_data)
{
  if (ctx->setting_data_list == NULL) {
    return;
  }

  setting_data_t *s;
  /* Find element before the one to remove */
  for (s = ctx->setting_data_list; s->next != NULL; s = s->next) {
    if (s->next != *setting_data) {
      continue;
    }

    struct setting_data_s *to_be_freed = s->next;

    *setting_data = NULL;
    s->next = s->next->next;

    setting_data_members_destroy(to_be_freed);
    free(to_be_freed);
  }
}

/**
 * @brief setting_create_setting - factory for new settings
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
 * @return the newly created setting, NULL if failed
 */
static setting_data_t *setting_create_setting(settings_t *ctx,
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
  /* Look up type data */
  type_data_t *type_data = type_data_lookup(ctx, type);
  if (type_data == NULL) {
    settings_api.log(log_err, "invalid type");
    return NULL;
  }

  /* Set up setting data */
  setting_data_t *setting_data = (setting_data_t *)malloc(sizeof(*setting_data));
  if (setting_data == NULL) {
    settings_api.log(log_err, "error allocating setting data");
    return NULL;
  }

  *setting_data = (setting_data_t){
    .section = strdup(section),
    .name = strdup(name),
    .var = var,
    .var_len = var_len,
    .var_copy = malloc(var_len),
    .type_data = type_data,
    .notify = notify,
    .notify_context = notify_context,
    .readonly = readonly,
    .watchonly = watchonly,
    .next = NULL,
  };

  if ((setting_data->section == NULL) || (setting_data->name == NULL)
      || (setting_data->var_copy == NULL)) {
    settings_api.log(log_err, "error allocating setting data members");
    setting_data_members_destroy(setting_data);
    free(setting_data);
    setting_data = NULL;
  }

  return setting_data;
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
  compare_init(ctx, message, header_length);

  uint8_t tries = 0;
  bool success = false;

  /* Prime semaphores etc if applicable */
  if (settings_api.wait_init) {
    settings_api.wait_init(settings_api.ctx);
  }

  do {
    settings_api.send_from(settings_api.ctx,
                           message_type,
                           message_length,
                           (uint8_t *)message,
                           sender_id);

    if (settings_api.wait(settings_api.ctx, timeout_ms)) {
      size_t len1 = strlen(message) + 1;
      settings_api.log(log_err,
                       "Waiting reply for msg id %d with %s.%s timed out",
                       message_type,
                       message,
                       message + len1);
    } else {
      success = compare_match(ctx);
    }

  } while (!success && (++tries < retries));

  /* Defuse semaphores etc if applicable */
  if (settings_api.wait_deinit) {
    settings_api.wait_deinit(settings_api.ctx);
  }

  compare_deinit(ctx);

  if (!success) {
    settings_api.log(log_err, "setting req/reply failed for msg id %d", message_type);
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
                                            sys_sender_id);
}

/**
 * @brief setting_register - perform SBP_MSG_SETTINGS_REGISTER req/reply
 * @param ctx: settings context
 * @param setting_data: setting to register with settings daemon
 * @return zero on success, -1 the transaction failed to complete
 */
static int setting_register(settings_t *ctx, setting_data_t *setting_data)
{
  /* Build message */
  char msg[SBP_PAYLOAD_SIZE_MAX];
  uint8_t msg_len = 0;
  uint8_t msg_header_len;
  int l;

  l = message_header_get(setting_data, &msg[msg_len], sizeof(msg) - msg_len);
  if (l < 0) {
    settings_api.log(log_err, "error building settings message");
    return -1;
  }
  msg_len += l;
  msg_header_len = msg_len;

  l = message_data_get(setting_data, &msg[msg_len], sizeof(msg) - msg_len);
  if (l < 0) {
    settings_api.log(log_err, "error building settings message");
    return -1;
  }
  msg_len += l;

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
  char msg[SBP_PAYLOAD_SIZE_MAX];
  uint8_t msg_len = 0;
  int l;

  if (!setting_data->watchonly) {
    settings_api.log(log_err, "cannot update non-watchonly setting manually");
    return -1;
  }

  l = message_header_get(setting_data, &msg[msg_len], sizeof(msg) - msg_len);
  if (l < 0) {
    settings_api.log(log_err, "error building settings read req message");
    return -1;
  }
  msg_len += l;

  if (settings_register_read_resp_callback(ctx) != 0) {
    settings_api.log(log_err, "error registering settings read callback");
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

/**
 * @brief members_destroy - deinit for settings context members
 * @param ctx: settings context to deinit
 */
static void members_destroy(settings_t *ctx)
{
  /* Free type data list elements */
  while (ctx->type_data_list != NULL) {
    type_data_t *t = ctx->type_data_list;
    ctx->type_data_list = ctx->type_data_list->next;
    free(t);
  }

  /* Free setting data list elements */
  while (ctx->setting_data_list != NULL) {
    setting_data_t *s = ctx->setting_data_list;
    ctx->setting_data_list = ctx->setting_data_list->next;
    setting_data_members_destroy(s);
    free(s);
  }
}

settings_t *settings_create(void)
{
  settings_t *ctx = (settings_t *)malloc(sizeof(*ctx));
  if (ctx == NULL) {
    settings_api.log(log_err, "error allocating context");
    return ctx;
  }

  ctx->type_data_list = NULL;
  ctx->setting_data_list = NULL;
  ctx->registration_state.pending = false;
  ctx->write_callback_registered = false;
  ctx->write_resp_callback_registered = false;
  ctx->read_resp_callback_registered = false;

  /* Register standard types */
  settings_type_t type;

  assert(type_register(ctx, int_to_string, int_from_string, NULL, NULL, &type) == 0);
  assert(type == SETTINGS_TYPE_INT);

  assert(type_register(ctx, float_to_string, float_from_string, NULL, NULL, &type) == 0);
  assert(type == SETTINGS_TYPE_FLOAT);

  assert(type_register(ctx, str_to_string, str_from_string, NULL, NULL, &type) == 0);
  assert(type == SETTINGS_TYPE_STRING);

  assert(
    type_register(ctx, enum_to_string, enum_from_string, enum_format_type, bool_enum_names, &type)
    == 0);
  assert(type == SETTINGS_TYPE_BOOL);

  return ctx;
}

void settings_destroy(settings_t **ctx)
{
  if (ctx == NULL || *ctx == NULL) {
    settings_api.log(log_err, "%s error: invalid pointer", __FUNCTION__);
    return;
  }

  members_destroy(*ctx);
  free(*ctx);
  *ctx = NULL;
}

int settings_register_enum(settings_t *ctx, const char *const enum_names[], settings_type_t *type)
{
  assert(ctx != NULL);
  assert(enum_names != NULL);
  assert(type != NULL);

  return type_register(ctx, enum_to_string, enum_from_string, enum_format_type, enum_names, type);
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

  if (setting_data_lookup(ctx, section, name) != NULL) {
    settings_api.log(log_err, "setting add failed - duplicate setting");
    return -1;
  }

  setting_data_t *setting_data = setting_create_setting(ctx,
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
    settings_api.log(log_err, "error creating setting data");
    return -1;
  }

  /* Add to list */
  setting_data_list_insert(ctx, setting_data);

  if (watchonly) {
    if (settings_register_write_resp_callback(ctx) != 0) {
      settings_api.log(log_err, "error registering settings write resp callback");
    }
    if (setting_read_watched_value(ctx, setting_data) != 0) {
      settings_api.log(log_err, "error reading watched %s.%s to initial value", section, name);
    }
  } else {
    if (settings_register_write_callback(ctx) != 0) {
      settings_api.log(log_err, "error registering settings write callback");
    }
    if (setting_register(ctx, setting_data) != 0) {
      settings_api.log(log_err, "error registering %s.%s with settings manager", section, name);
      setting_data_list_remove(ctx, &setting_data);
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
