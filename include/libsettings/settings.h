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

#include <libsettings/settings_declspec.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Settings type.
 */
typedef int settings_type_t;

/**
 * @struct  settings_t
 *
 * @brief   Opaque context for settings.
 */
typedef struct settings_s settings_t;

#define SETTINGS_BUFLEN 255
#define READ_BY_INDEX_OFFSET 3

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
 * @brief Settings register error codes
 */
typedef enum settings_reg_res_e {
  SETTINGS_REG_OK = 0, /**< Setting registered, requested value used */
  SETTINGS_REG_OK_PERM =
      1, /**< Setting registered, permanent storage value found and returned */
  SETTINGS_REG_REGISTERED =
      2, /**< Setting is already registered, value from memory returned */
  SETTINGS_REG_PARSE_FAILED = 3, /**< Could not parse setting */
} settings_reg_res_t;

/**
 * @brief Settings write error codes
 */
typedef enum settings_write_res_e {
  SETTINGS_WR_OK = 0,               /**< Setting written               */
  SETTINGS_WR_VALUE_REJECTED = 1,   /**< Setting value invalid   */
  SETTINGS_WR_SETTING_REJECTED = 2, /**< Setting does not exist */
  SETTINGS_WR_PARSE_FAILED = 3,     /**< Could not parse setting value */
  /* READ_ONLY:MODIFY_DISABLED ~= Permanent:Temporary */
  SETTINGS_WR_READ_ONLY = 4,       /**< Setting is read only          */
  SETTINGS_WR_MODIFY_DISABLED = 5, /**< Setting is not modifiable     */
  SETTINGS_WR_SERVICE_FAILED = 6,  /**< System failure during setting */
  SETTINGS_WR_TIMEOUT = 7,         /**< Request wasn't replied in time */
} settings_write_res_t;

typedef int (*settings_send_t)(void *ctx, uint16_t msg_type, uint8_t len,
                               uint8_t *payload);
typedef int (*settings_send_from_t)(void *ctx, uint16_t msg_type, uint8_t len,
                                    uint8_t *payload, uint16_t sbp_sender_id);

typedef int (*settings_wait_init_t)(void *ctx);
typedef int (*settings_wait_t)(void *ctx, int timeout_ms);
typedef int (*settings_wait_deinit_t)(void *ctx);
typedef void (*settings_signal_t)(void *ctx);

typedef int (*settings_wait_thd_t)(void *event, int timeout_ms);
typedef void (*settings_signal_thd_t)(void *event);

typedef void (*settings_lock_t)(void *ctx);
typedef void (*settings_unlock_t)(void *ctx);

typedef int (*settings_reg_cb_t)(void *ctx, uint16_t msg_type,
                                 sbp_msg_callback_t cb, void *cb_context,
                                 sbp_msg_callbacks_node_t **node);
typedef int (*settings_unreg_cb_t)(void *ctx, sbp_msg_callbacks_node_t **node);

typedef void (*settings_log_t)(int priority, const char *format, ...);
typedef void (*settings_log_preformatted_t)(int priority, const char *format);

typedef struct settings_api_s {
  void *ctx;
  settings_send_t send;
  settings_send_from_t send_from;
  settings_wait_init_t
      wait_init; /* Optional, needed if wait uses semaphores etc */
  settings_wait_t wait;
  settings_wait_deinit_t
      wait_deinit; /* Optional, needed if wait uses semaphores etc */
  settings_signal_t signal;
  settings_wait_t wait_thd;     /* Required for multithreading */
  settings_signal_t signal_thd; /* Required for multithreading */
  settings_lock_t lock;         /* Required for multithreading */
  settings_unlock_t unlock;     /* Required for multithreading */
  settings_reg_cb_t register_cb;
  settings_unreg_cb_t unregister_cb;
  settings_log_t log;
  settings_log_preformatted_t log_preformatted;
} settings_api_t;

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
 * @brief   Create a settings context.
 * @details Create and initialize a settings context.
 *
 * @return                  Pointer to the created context, or NULL if the
 *                          operation failed.
 */
LIBSETTINGS_DECLSPEC settings_t *settings_create(uint16_t sender_id,
                                                 settings_api_t *api_impl);

/**
 * @brief   Destroy a settings context.
 * @details Deinitialize and destroy a settings context.
 *
 * @note    The context pointer will be set to NULL by this function.
 *
 * @param[inout] ctx        Double pointer to the context to destroy.
 */
LIBSETTINGS_DECLSPEC void settings_destroy(settings_t **ctx);

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
LIBSETTINGS_DECLSPEC int settings_register_enum(settings_t *ctx,
                                                const char *const enum_names[],
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
LIBSETTINGS_DECLSPEC int settings_register_setting(
    settings_t *ctx, const char *section, const char *name, void *var,
    size_t var_len, settings_type_t type, settings_notify_fn notify,
    void *notify_context);

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
LIBSETTINGS_DECLSPEC int settings_register_readonly(
    settings_t *ctx, const char *section, const char *name, const void *var,
    size_t var_len, settings_type_t type);

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
LIBSETTINGS_DECLSPEC int settings_register_watch(
    settings_t *ctx, const char *section, const char *name, void *var,
    size_t var_len, settings_type_t type, settings_notify_fn notify,
    void *notify_context);

/**
 * @brief   Write a new value for registered setting.
 * @details Call will block until write response or internal timeout.
 *          In case of multithreading, caller shall provide event object
 *          that shall be used for waiting in wait_thd API function and
 *          signaled in signal_thd API function. Also lock and unlock API
 *          function implementations are mandatory when multithreading.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] event         Request specific event object.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] value         Address of the value variable.
 * @param[in] value_len     Size of the value variable.
 * @param[in] type          Type of the setting.
 *
 * @return                  The operation result.
 * @retval -1               Sending of the request failed.
 * @retval 0                The setting was written successfully.
 * @retval >0               Response returned an error @see settings_write_res_t
 */
LIBSETTINGS_DECLSPEC settings_write_res_t settings_write(
    settings_t *ctx, void *event, const char *section, const char *name,
    const void *value, size_t value_len, settings_type_t type);

/**
 * @brief   Write a new value for registered setting of type int.
 * @details Call will block until write response or internal timeout.
 *          In case of multithreading, caller shall provide event object
 *          that shall be used for waiting in wait_thd API function and
 *          signaled in signal_thd API function. Also lock and unlock API
 *          function implementations are mandatory when multithreading.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] event         Request specific event object.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] value         Value to be written.
 *
 * @return                  The operation result.
 * @retval -1               Sending of the request failed.
 * @retval 0                The setting was written successfully.
 * @retval >0               Response returned an error @see settings_write_res_t
 */
LIBSETTINGS_DECLSPEC settings_write_res_t
settings_write_int(settings_t *ctx, void *event, const char *section,
                   const char *name, int value);

/**
 * @brief   Write a new value for registered setting of type float.
 * @details Call will block until write response or internal timeout.
 *          In case of multithreading, caller shall provide event object
 *          that shall be used for waiting in wait_thd API function and
 *          signaled in signal_thd API function. Also lock and unlock API
 *          function implementations are mandatory when multithreading.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] event         Request specific event object.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] value         Value to be written.
 *
 * @return                  The operation result.
 * @retval -1               Sending of the request failed.
 * @retval 0                The setting was written successfully.
 * @retval >0               Response returned an error @see settings_write_res_t
 */
LIBSETTINGS_DECLSPEC settings_write_res_t
settings_write_float(settings_t *ctx, void *event, const char *section,
                     const char *name, float value);

/**
 * @brief   Write a new value for registered setting of type str.
 * @details Call will block until write response or internal timeout.
 *          In case of multithreading, caller shall provide event object
 *          that shall be used for waiting in wait_thd API function and
 *          signaled in signal_thd API function. Also lock and unlock API
 *          function implementations are mandatory when multithreading.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] event         Request specific event object.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] str           Value to be written.
 *
 * @return                  The operation result.
 * @retval -1               Sending of the request failed.
 * @retval 0                The setting was written successfully.
 * @retval >0               Response returned an error @see settings_write_res_t
 */
LIBSETTINGS_DECLSPEC settings_write_res_t
settings_write_str(settings_t *ctx, void *event, const char *section,
                   const char *name, const char *str);

/**
 * @brief   Write a new value for registered setting of type bool.
 * @details Call will block until write response or internal timeout.
 *          In case of multithreading, caller shall provide event object
 *          that shall be used for waiting in wait_thd API function and
 *          signaled in signal_thd API function. Also lock and unlock API
 *          function implementations are mandatory when multithreading.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] event         Request specific event object.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] value         Value to be written.
 *
 * @return                  The operation result.
 * @retval -1               Sending of the request failed.
 * @retval 0                The setting was written successfully.
 * @retval >0               Response returned an error @see settings_write_res_t
 */
LIBSETTINGS_DECLSPEC settings_write_res_t
settings_write_bool(settings_t *ctx, void *event, const char *section,
                    const char *name, bool value);

/**
 * @brief   Read value of registered setting.
 * @details Call will block until read response or internal timeout.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[in] value         Address of the variable where the read value shall
 * be written.
 * @param[in] value_len     Size of the value variable.
 * @param[in] type          Type of the setting.
 *
 * @return                  The operation result.
 * @retval 0                The setting was read successfully. Error otherwise.
 */
LIBSETTINGS_DECLSPEC int settings_read(settings_t *ctx, const char *section,
                                       const char *name, void *value,
                                       size_t value_len, settings_type_t type);

/**
 * @brief   Read value of registered setting of type int.
 * @details Call will block until read response or internal timeout.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[out] value        Address of the variable where the read value shall
 * be written.
 *
 * @return                  The operation result.
 * @retval 0                The setting was read successfully. Error otherwise.
 */
LIBSETTINGS_DECLSPEC int settings_read_int(settings_t *ctx, const char *section,
                                           const char *name, int *value);

/**
 * @brief   Read value of registered setting of type float.
 * @details Call will block until read response or internal timeout.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[out] value        Address of the variable where the read value shall
 * be written.
 *
 * @return                  The operation result.
 * @retval 0                The setting was read successfully. Error otherwise.
 */
LIBSETTINGS_DECLSPEC int settings_read_float(settings_t *ctx,
                                             const char *section,
                                             const char *name, float *value);

/**
 * @brief   Read value of registered setting of type str.
 * @details Call will block until read response or internal timeout.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[out] str          Address of the variable where the read value shall
 * be written.
 * @param[in] str_len       Size of the str buffer.
 *
 * @return                  The operation result.
 * @retval 0                The setting was read successfully. Error otherwise.
 */
LIBSETTINGS_DECLSPEC int settings_read_str(settings_t *ctx, const char *section,
                                           const char *name, char *str,
                                           size_t str_len);

/**
 * @brief   Read value of registered setting of type bool.
 * @details Call will block until read response or internal timeout.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] section       String describing the setting section.
 * @param[in] name          String describing the setting name.
 * @param[out] value        Address of the variable where the read value shall
 * be written.
 *
 * @return                  The operation result.
 * @retval 0                The setting was read successfully. Error otherwise.
 */
LIBSETTINGS_DECLSPEC int settings_read_bool(settings_t *ctx,
                                            const char *section,
                                            const char *name, bool *value);

/**
 * @brief   Read value of registered setting based on index.
 * @details Call will block until read_by_idx response or internal timeout.
 *
 * @param[in] ctx           Pointer to the context to use.
 * @param[in] event         Request specific event object.
 * @param[in] idx           Index to read.
 * @param[out] section      String describing the setting section.
 * @param[in] section_len   Section str buffer size.
 * @param[out] name         String describing the setting name.
 * @param[in] name_len      Name str buffer size.
 * @param[out] value        String describing the setting value.
 * @param[in] value_len     Value str buffer size.
 * @param[out] type         String describing the setting type.
 * @param[in] type_len      Type str buffer size.
 *
 * @return                  The operation result.
 * @retval 0                The setting was read successfully. Next index is
 * ready to be read.
 * @retval <0               Error.
 * @retval >0               Last index was read successfully. There are no more
 * indexes to read.
 */
LIBSETTINGS_DECLSPEC int settings_read_by_idx(settings_t *ctx, void *event,
                                              uint16_t idx, char *section,
                                              size_t section_len, char *name,
                                              size_t name_len, char *value,
                                              size_t value_len, char *type,
                                              size_t type_len);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTINGS_H */
