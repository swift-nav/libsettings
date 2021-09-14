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

#ifndef LIBSETTINGS_SETTING_MACROS_H
#define LIBSETTINGS_SETTING_MACROS_H

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

#endif
