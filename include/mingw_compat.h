#pragma once
#if defined(_WIN32)
#if defined(__MINGW32__) || defined(__clang__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <timeapi.h>
#endif
#endif
