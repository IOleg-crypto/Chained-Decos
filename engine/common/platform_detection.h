#ifndef CH_PLATFORM_DETECTION_H
#define CH_PLATFORM_DETECTION_H

#define CH_PLATFORM_WINDOWS 0
#define CH_PLATFORM_LINUX 0
#define CH_PLATFORM_MACOS 0
#define CH_PLATFORM_IOS 0
#define CH_PLATFORM_ANDROID 0

#if defined(_WIN32)
#ifndef _WIN64
#error "x86 Builds are not supported!"
#endif
#undef CH_PLATFORM_WINDOWS
#define CH_PLATFORM_WINDOWS 1

#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
#error "iOS / Simulator is not supported!"
#elif TARGET_OS_OSX == 1
#error "MacOS is not supported!"
#else
#error "Unknown Apple platform!"
#endif

#elif defined(__ANDROID__)
#error "Android is not supported!"

#elif defined(__linux__)
#undef CH_PLATFORM_LINUX
#define CH_PLATFORM_LINUX 1

#else
#error "Unknown platform!"
#endif

#endif // CH_PLATFORM_DETECTION_H