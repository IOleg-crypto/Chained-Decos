#ifndef CH_CORE_EXPORT_H
#define CH_CORE_EXPORT_H

#include "engine/core/platform_detection.h"

#ifdef CH_PLATFORM_WINDOWS
    #ifdef CH_ENGINE_SHARED
        #ifdef CH_ENGINE_BUILD
            #define CH_API __declspec(dllexport)
        #else
            #define CH_API __declspec(dllimport)
        #endif
    #else
        #define CH_API
    #endif
#else
    #define CH_API
#endif

#endif // CH_CORE_EXPORT_H
