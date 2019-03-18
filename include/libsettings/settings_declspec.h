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

#ifndef LIBSETTINGS_SETTINGS_DECLSPEC_H
#define LIBSETTINGS_SETTINGS_DECLSPEC_H

#if defined(_MSC_VER)
#if !defined(_WINDLL)
/* Leave empty when doing non-dll build */
#define LIBSETTINGS_DECLSPEC
#elif defined(settings_EXPORTS) /* settings_EXTENSION */
#define LIBSETTINGS_DECLSPEC __declspec(dllexport)
#else /* settings_EXPORTS */
#define LIBSETTINGS_DECLSPEC __declspec(dllimport)
#endif                  /* settings_EXPORTS */
#elif defined(__GNUC__) /* _MSC_VER */
#define LIBSETTINGS_DECLSPEC __attribute__((visibility("default")))
#else /* __GNUC__ */
#pragma error Unknown dynamic link import / export semantics.
#endif /* __GNUC__ */

#endif /* LIBSETTINGS_SETTINGS_DECLSPEC_H */
