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

#ifndef LIBSETTINGS_SETTING_DEF_H
#define LIBSETTINGS_SETTING_DEF_H

#include <libsettings/settings.h>

#include <internal/setting_data.h>
#include <internal/setting_type.h>

typedef struct setting_sbp_cb_s setting_sbp_cb_t;

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
  request_state_t *req_list;
  setting_sbp_cb_t *sbp_cb_list;
  settings_api_t client_iface;
  uint16_t sender_id;
};

#endif /* LIBSETTINGS_SETTING_DEF_H */
