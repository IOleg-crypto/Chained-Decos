# GLFW dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/glfw/CMakeLists.txt")
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    # On Linux, GLFW builds both X11 and Wayland backends by default. The Wayland
    # backend requires wayland-scanner + libwayland-dev at configure time, which we
    # don't ship on CI (or most dev machines). X11 alone is sufficient for our
    # OpenGL renderer, so disable Wayland to avoid a hard configure failure.
    if(UNIX AND NOT APPLE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    endif()

    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/glfw" "${CMAKE_BINARY_DIR}/vendor/glfw" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "GLFW submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/glfw")
endif()
