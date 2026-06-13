# ============================================================================
# Chained Engine - Dependencies Configuration
# ============================================================================

# Add external modules directory to search path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/external")

# Core dependencies (independent)
include(yaml-cpp)
include(glm)
include(entt)
include(miniaudio)
include(stb)
include(cereal)
include(zstd)
include(spdlog)
include(jolt)

# Platform & Graphics (independent)
include(glfw)
include(glad)

# UI (depends on GLFW and GLAD)
include(imgui)

# Complex modules
include(assimp)
include(coral)
include(nfd)
# No time for networking right now

#include(protobuf) # Required for GNS
#include(gamenetworkingsockets)

include(reflect-cpp)

# Tests
if(BUILD_TESTS)
    include(external_gtest)
endif()
