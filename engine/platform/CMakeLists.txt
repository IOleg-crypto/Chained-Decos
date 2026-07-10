# GLFW is the only supported window backend
add_subdirectory(backends/glfw)
set(CH_PLATFORM_BACKEND_TARGET engine_platform_glfw)

add_subdirectory(dialogs)

# Unified platform target: backend + dialogs
add_library(engine_platform INTERFACE)
target_link_libraries(engine_platform INTERFACE
    ${CH_PLATFORM_BACKEND_TARGET}
    engine_platform_dialogs
)
