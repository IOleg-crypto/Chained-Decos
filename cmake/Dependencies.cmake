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
# zstd is provided by pack — no separate include needed.
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
include(external_gtest)
include(pack)
include(portable-file-dialogs)
# GameNetworkingSockets replaces enet (transport, encryption; NAT traversal is not wired up)
include(reflect-cpp)
include(miniupnpc)

# Disable unity builds for third-party libraries to avoid symbol redefinitions
# (e.g., zstd cover.h has no include guard, causing redefinition under unity build)
# Coral.Native is specifically excluded because MSVC's unity PCH in C++20 mode
# deletes operator<<(wchar_t*) which is used internally by Coral's cerr logging.
foreach(_ext_target
    libzstd_static yaml-cpp
    glm entt cereal stb spdlog miniaudio
    imgui imguizmo
    glfw glad
    Jolt
    GTest gmock
    Coral.Native
    assimp


)
    if(TARGET ${_ext_target})
        set_target_properties(${_ext_target} PROPERTIES UNITY_BUILD OFF)
    endif()
endforeach()

if(NOT TARGET libzstd_static AND NOT TARGET libzstd)
    add_subdirectory(thirdparty/zstd/build/cmake/lib)
endif()
