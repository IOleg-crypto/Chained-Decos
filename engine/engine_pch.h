#ifndef ENGINE_PCH_H
#define ENGINE_PCH_H

// 1. Assertion setup & Platform Detection
#include <cassert>
#include <cstdint>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// GLM behavior macros (GLM_ENABLE_EXPERIMENTAL, GLM_FORCE_DEPTH_ZERO_TO_ONE)
// are defined globally in the root CMakeLists.txt so that TUs compiled without
// this PCH (NO_PCH modules) see the same GLM configuration.

// 2. Platform specific
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// 3. Core Engine Headers (Essential for all files)
#include "engine/common/base.h"
#include "engine/core/log.h"
#include "engine/common/color.h"

// 4. Vendor libraries
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <yaml-cpp/yaml.h>

#endif // ENGINE_PCH_H
