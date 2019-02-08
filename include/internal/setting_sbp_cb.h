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

#ifndef LIBSETTINGS_SETTING_SBP_CB_H
#define LIBSETTINGS_SETTING_SBP_CB_H

#include <libsbp/sbp.h>

#include <libsettings/settings.h>

typedef struct setting_sbp_cb_s {
  uint16_t msg_id;
  sbp_msg_callback_t cb;
  sbp_msg_callbacks_node_t *cb_node;
  struct setting_sbp_cb_s *next;
} setting_sbp_cb_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Register a callback function for specific SBP message
 *
 * @param[in,out] ctx       Settings context
 * @param[in] msg_id        SBP message ID to register
 *
 * @return                  0 on success
 *                          1 if callback is already registered
 *                         -1 on error
 */
int setting_sbp_cb_register(settings_t *ctx, uint16_t msg_id);

/**
 * @brief   Unegister a callback function for specific SBP message
 *
 * @param[in,out] ctx       Settings context
 * @param[in] msg_id        SBP message ID to unregister
 *
 * @return                  0 on success
 *                          1 if callback is not registered
 *                         -1 on error
 */
int setting_sbp_cb_unregister(settings_t *ctx, uint16_t msg_id);

#ifdef __cplusplus
}
#endif

#endif /* LIBSETTINGS_SETTING_SBP_CB_H */
