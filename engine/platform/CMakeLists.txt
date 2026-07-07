set(CH_PLATFORM_BACKEND "glfw" CACHE STRING "Window backend: glfw or sdl3")
set_property(CACHE CH_PLATFORM_BACKEND PROPERTY STRINGS "glfw" "sdl3")

if(CH_PLATFORM_BACKEND STREQUAL "glfw")
    add_subdirectory(backends/glfw)
    set(CH_PLATFORM_BACKEND_TARGET engine_platform_glfw)
elseif(CH_PLATFORM_BACKEND STREQUAL "sdl3")
    add_subdirectory(backends/sdl3)
    set(CH_PLATFORM_BACKEND_TARGET engine_platform_sdl3)
else()
    message(FATAL_ERROR "Unknown platform backend: ${CH_PLATFORM_BACKEND}. Use 'glfw' or 'sdl3'.")
endif()

add_subdirectory(dialogs)

# Unified platform target: backend + dialogs
add_library(engine_platform INTERFACE)
target_link_libraries(engine_platform INTERFACE
    ${CH_PLATFORM_BACKEND_TARGET}
    engine_platform_dialogs
)

message(STATUS "Platform backend: ${CH_PLATFORM_BACKEND}")
