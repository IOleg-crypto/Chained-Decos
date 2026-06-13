#ifndef MINGW_COMPAT_H
#define MINGW_COMPAT_H
#if defined(_WIN32)
#if defined(__MINGW32__) || defined(__clang__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <timeapi.h>
#include <windows.h>

#endif
#endif
#endif
