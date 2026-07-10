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
include(external_gtest)
# No time for networking right now

# include(protobuf) # Required for GNS
# include(gamenetworkingsockets)

include(reflect-cpp)

# Disable unity builds for third-party libraries to avoid symbol redefinitions
# (e.g., zstd cover.h has no include guard, causing redefinition under unity build)
foreach(_ext_target
    libzstd_static yaml-cpp
    glm entt cereal stb spdlog miniaudio
    imgui imguizmo
    glfw glad
    Jolt
    nfd
    GTest gmock
)
    if(TARGET ${_ext_target})
        set_target_properties(${_ext_target} PROPERTIES UNITY_BUILD OFF)
    endif()
endforeach()
