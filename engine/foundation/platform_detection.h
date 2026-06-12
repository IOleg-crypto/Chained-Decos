#ifndef CH_PLATFORM_DETECTION_H
#define CH_PLATFORM_DETECTION_H

// Platform detection using predefined macros
#if defined(_WIN32)
    #define CH_PLATFORM_WINDOWS 1
    #define CH_PLATFORM_LINUX 0
    #define CH_PLATFORM_MACOS 0
    #define CH_PLATFORM_IOS 0
    #define CH_PLATFORM_ANDROID 0
    #ifndef _WIN64
        #error "x86 Builds are not supported!"
    #endif
#elif defined(__APPLE__) || defined(__MACH__)
    #include <TargetConditionals.h>
    #define CH_PLATFORM_WINDOWS 0
    #define CH_PLATFORM_LINUX 0
    #define CH_PLATFORM_ANDROID 0
    #if TARGET_IPHONE_SIMULATOR == 1
        #error "IOS simulator is not supported!"
    #elif TARGET_OS_IPHONE == 1
        #define CH_PLATFORM_MACOS 0
        #define CH_PLATFORM_IOS 1
        #error "IOS is not supported!"
    #elif TARGET_OS_MAC == 1
        #define CH_PLATFORM_MACOS 1
        #define CH_PLATFORM_IOS 0
        #error "MacOS is not supported!"
    #else
        #error "Unknown Apple platform!"
    #endif
#elif defined(__ANDROID__)
    #define CH_PLATFORM_WINDOWS 0
    #define CH_PLATFORM_LINUX 1 // Android is Linux-based
    #define CH_PLATFORM_MACOS 0
    #define CH_PLATFORM_IOS 0
    #define CH_PLATFORM_ANDROID 1
    #error "Android is not supported!"
#elif defined(__linux__)
    #define CH_PLATFORM_WINDOWS 0
    #define CH_PLATFORM_LINUX 1
    #define CH_PLATFORM_MACOS 0
    #define CH_PLATFORM_IOS 0
    #define CH_PLATFORM_ANDROID 0
#else
    #error "Unknown platform!"
#endif

#endif // CH_PLATFORM_DETECTION_H
