# ============================================================================
# Chained Engine - Dependencies Configuration
# ============================================================================

# Disable tests inside third-party libraries — prevents Nightly*/CTest target pollution.
# These must be set BEFORE add_subdirectory / include() of each library.
set(YAML_CPP_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLM_TEST_ENABLE         OFF CACHE BOOL "" FORCE)
set(ENTT_BUILD_TESTING      OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING           OFF CACHE BOOL "" FORCE)

# Add external modules directory to search path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/external")

# Core dependencies (independent)
include(yaml-cpp)
include(glm)
include(entt)
include(miniaudio)
include(cereal)
include(stb)
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

# include(protobuf) # Required for GNS
# include(gamenetworkingsockets)

include(reflect-cpp)

# Tests
if(BUILD_TESTS)
    include(external_gtest)
endif()
