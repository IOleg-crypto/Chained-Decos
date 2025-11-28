add_library(module STATIC
    Core/ModuleManager.h
    Core/ModuleManager.cpp
    Core/ModuleContext.h
    Interfaces/IEngineModule.h
)

target_include_directories(module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(module PUBLIC
    event
    raylib
)

# Add raylib include directories for FetchContent
if(raylib_POPULATED)
    target_include_directories(module PUBLIC ${raylib_SOURCE_DIR}/src)
    message(STATUS "Module: Using raylib from FetchContent at ${raylib_SOURCE_DIR}/src")
elseif(NOT raylib_FOUND)
    message(WARNING "Module: raylib not found and not populated via FetchContent")
endif()

# Ensure raylib headers are available
if(TARGET raylib)
    get_target_property(raylib_INCLUDE_DIRS raylib INTERFACE_INCLUDE_DIRECTORIES)
    if(raylib_INCLUDE_DIRS)
        target_include_directories(module PUBLIC ${raylib_INCLUDE_DIRS})
        message(STATUS "Module: Using raylib include dirs: ${raylib_INCLUDE_DIRS}")
    endif()
endif()

set_target_properties(module PROPERTIES POSITION_INDEPENDENT_CODE ON)
