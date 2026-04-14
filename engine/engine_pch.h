#ifndef ENGINE_PCH_H
#define ENGINE_PCH_H

// 1. Assertion setup (must be before any headers that might use them like GLM or EnTT)
#include <cassert>
#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// Fallback for assert if it's missing from <cassert> in this environment
#ifndef assert
    #ifdef _DEBUG
        #define assert(condition) ((condition) ? (void)0 : __builtin_trap())
    #else
        #define assert(condition) ((void)0)
    #endif
#endif

#ifdef _DEBUG
    #define CH_ASSERT_IMPL(condition) assert(condition)
#else
    #define CH_ASSERT_IMPL(condition) ((void)0)
#endif

#ifndef ENTT_ASSERT
    #define ENTT_ASSERT(condition, msg) CH_ASSERT_IMPL(condition)
#endif
#ifndef ENTT_ASSERT_CONSTEXPR
    #define ENTT_ASSERT_CONSTEXPR(condition, msg) CH_ASSERT_IMPL(condition)
#endif

#ifndef GLM_ASSERT
    #define GLM_ASSERT(x) CH_ASSERT_IMPL(x)
#endif

// 2. Standard Library
#include "engine/core/ch_math.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <fstream>
#include <sstream>

// 4. Platform specific
#ifdef WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

// 5. Vendor libraries
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#endif // ENGINE_PCH_H
