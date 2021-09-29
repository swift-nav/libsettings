#ifndef LIBSETTINGS_WRAPPER_H
#define LIBSETTINGS_WRAPPER_H

typedef unsigned char uint8_t;
typedef signed char int8_t;

typedef unsigned short uint16_t;
typedef signed short int16_t;

typedef unsigned int uint32_t;
typedef signed int int32_t;

#if defined(__GNUC__) && defined(__linux__)
typedef int int64_t __attribute__((__mode__(__DI__)));
typedef unsigned long int uint64_t;
#else
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif

#define UINT8_MAX 255

typedef int8_t _s8;
typedef int16_t _s16;
typedef int32_t _s32;
typedef int64_t _s64;
typedef uint8_t _u8;
typedef uint16_t _u16;
typedef uint32_t _u32;
typedef uint64_t _u64;

#define s8 _s8
#define s16 _s16
#define s32 _s32
#define s64 _s64
#define u8 _u8
#define u16 _u16
#define u32 _u32
#define u64 _u64

#ifndef bool
#define bool _Bool
#endif

#ifndef _BUILD_RUSTBIND_LIB_
/* Put stuff here that shouldn't be seen during rustbindsettings build */
#endif  //_BUILD_RUSTBIND_LIB_

#define _RUSTC_BINDGEN_ 1
#include "libsettings/settings.h"

#endif
