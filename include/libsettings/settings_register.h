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
 * @file    settings.h
 * @brief   Settings API.
 *
 * @defgroup    settings Settings
 * @addtogroup  settings
 * @{
 */

#ifndef LIBSETTINGS_SETTINGS_H
#define LIBSETTINGS_SETTINGS_H

#include <inttypes.h>
#include <stddef.h>

#include <libsbp/sbp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*settings_send_t)(void *ctx, uint16_t msg_type, uint8_t len,
                               uint8_t *payload);
typedef int (*settings_send_from_t)(void *ctx, uint16_t msg_type, uint8_t len,
                                    uint8_t *payload, uint16_t sbp_sender_id);

typedef int (*settings_wait_init_t)(void *ctx);
typedef int (*settings_wait_t)(void *ctx, int timeout_ms);
typedef int (*settings_wait_deinit_t)(void *ctx);
typedef void (*settings_signal_t)(void *ctx);

typedef int (*settings_reg_cb_t)(void *ctx, uint16_t msg_type,
                                 sbp_msg_callback_t cb, void *cb_context,
                                 sbp_msg_callbacks_node_t **node);
typedef int (*settings_unreg_cb_t)(void *ctx, sbp_msg_callbacks_node_t **node);

typedef void (*settings_log_t)(int priority, const char *format, ...);

typedef struct setreg_api_s {
  void *ctx;
  settings_send_t send;
  settings_send_from_t send_from;
  settings_wait_init_t
      wait_init; /* Optional, needed if wait uses semaphores etc */
  settings_wait_t wait;
  settings_wait_deinit_t
      wait_deinit; /* Optional, needed if wait uses semaphores etc */
  settings_signal_t signal;
  settings_reg_cb_t register_cb;
  settings_unreg_cb_t unregister_cb;
  settings_log_t log;
} setreg_api_t;

void setreg_api_init(const setreg_api_t *api);

/**
 * @brief   Settings type.
 */
typedef int settings_type_t;

/**
 * @brief   Standard settings type definitions.
 */
enum {
  SETTINGS_TYPE_INT,    /**< Integer. 8, 16, or 32 bits.                   */
  SETTINGS_TYPE_FLOAT,  /**< Float. Single or double precision.            */
  SETTINGS_TYPE_STRING, /**< String.                                       */
  SETTINGS_TYPE_BOOL    /**< Boolean.                                      */
};

/**
 * @brief Settings error codes
 */
enum {
  SBP_SETTINGS_WRITE_STATUS_OK = 0, /**< Setting written               */
  SBP_SETTINGS_WRITE_STATUS_VALUE_REJECTED = 1,   /**< Setting value invalid   */
  SBP_SETTINGS_WRITE_STATUS_SETTING_REJECTED = 2, /**< Setting does not exist */
  SBP_SETTINGS_WRITE_STATUS_PARSE_FAILED =
      3, /**< Could not parse setting value */
  // READ_ONLY:MODIFY_DISABLED ~= Permanent:Temporary
  SBP_SETTINGS_WRITE_STATUS_READ_ONLY = 4, /**< Setting is read only          */
  SBP_SETTINGS_WRITE_STATUS_MODIFY_DISABLED =
      5, /**< Setting is not modifiable     */
  SBP_SETTINGS_WRITE_STATUS_SERVICE_FAILED =
      6, /**< System failure during setting */
};

void settings_sender_id_set(uint16_t id);

/**
 * @brief   Settings notify callback.
 * @details Signature of a user-provided callback function to be executed
 *          after a setting value is updated.
 *
 * @note    The setting value will be updated _before_ the callback is executed.
 *          If the callback returns an error, the setting value will be
 *          reverted to the previous value.
 *
 * @param[in] context       Pointer to the user-provided context.
 *
 * @return                  The operation result.
 * @retval 0                Success. The updated setting value is acceptable.
 * @retval -1               The updated setting value should be reverted.
 */
typedef int (*settings_notify_fn)(void *context);

/**
 * @struct  setreg_t
 *
 * @brief   Opaque context for settings.
 */
typedef struct setreg_s setreg_t;

/**
 * @brief   Create a settings context.
 * @details Create and initialize a settings context.
 *
 * @return                  Pointer to the created context, or NULL if the
 *                          operation failed.
 */
setreg_t *setreg_create(void);

/**
 * @brief   Destroy a settings context.
 * @details Deinitialize and destroy a settings context.
 *
 * @note    The context pointer will be set to NULL by this function.
 *
 * @param[inout] ctx        Double pointer to the context to destroy.
 */
void setreg_destroy(setreg_t **ctx);

/**
 * @brief   Register an enum type.
 * @details Register an enum as a settings type.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] enum_names    Null-terminated array of strings corresponding to
 *                          the possible enum values.
 * @param[out] type         Pointer to be set to the allocated settings type.
 *
 * @return                  The operation result.
 * @retval 0                The enum type was registered successfully.
 * @retval -1               An error occurred.
 */
int setreg_add_enum(setreg_t *ctx, const char *const enum_names[],
                    settings_type_t *type);

/**
 * @brief   Register a setting.
 * @details Register a persistent, user-facing setting.
 *
 * @note    The specified notify function will be executed from this function
 *          during initial registration.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] var           Address of the setting variable. This location will
 *                          be written directly by the settings module.
 * @param[in] var_len       Size of the setting variable.
 * @param[in] type          Type of the setting.
 * @param[in] notify        Notify function to be executed when the setting is
 *                          written and during initial registration.
 * @param[in] notify_context Context passed to the notify function.
 *
 * @return                  The operation result.
 * @retval 0                The setting was registered successfully.
 * @retval -1               An error occurred.
 */
int setreg_add_setting(setreg_t *ctx, const char *section, const char *name,
                       void *var, size_t var_len, settings_type_t type,
                       settings_notify_fn notify, void *notify_context);

/**
 * @brief   Register a read-only setting.
 * @details Register a read-only, user-facing setting.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] var           Address of the setting variable. This location will
 *                          be written directly by the settings module.
 * @param[in] var_len       Size of the setting variable.
 * @param[in] type          Type of the setting.
 *
 * @return                  The operation result.
 * @retval 0                The setting was registered successfully.
 * @retval -1               An error occurred.
 */
int setreg_add_readonly(setreg_t *ctx, const char *section, const char *name,
                        const void *var, size_t var_len, settings_type_t type);

/**
 * @brief   Create and add a watch only setting.
 * @details Create and add a watch only setting.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] var           Address of the setting variable. This location will
 *                          be written directly by the settings module.
 * @param[in] var_len       Size of the setting variable.
 * @param[in] type          Type of the setting.
 * @param[in] notify        Notify function to be executed when the setting is
 *                          updated by a write response
 * @param[in] notify_context Context passed to the notify function.
 *
 * @return                  The operation result.
 * @retval 0                The setting was registered successfully.
 * @retval -1               An error occurred.
 */
int setreg_add_watch(setreg_t *ctx, const char *section, const char *name,
                     void *var, size_t var_len, settings_type_t type,
                     settings_notify_fn notify, void *notify_context);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTINGS_H */

/** @} */
