add_library(engine_core ${SUBMODULE_LIB_TYPE}
        layer_stack.cpp
        profiler.cpp
        uuid.cpp
        input.cpp
        imgui_layer.cpp
        thread_pool.cpp
        application.cpp
        project_launcher.cpp
        assets/asset_manager.cpp
        application.h
        project_launcher.h
        imgui_layer.h
        base.h
        ch_assert.h
        platform_detection.h
        thread_pool.h
        entry_point.h
        events.h
        input.h
        layer.h
        layer_stack.h
        log.h
        profiler.h
        timestep.h
        uuid.h
        window.h
        service_locator.h
        ../platform/utils/dialogs.cpp
        ../platform/utils/dialogs.h
        ../platform/glfw/glfw_window.cpp
        ../platform/glfw/glfw_window.h
)

target_include_directories(engine_core PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/glfw/include
    ${CMAKE_SOURCE_DIR}/include/assimp/include
    ${CMAKE_BINARY_DIR}/include/assimp/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/stb
    ${CMAKE_SOURCE_DIR}/include/miniaudio
)

# Core module uses yaml-cpp, glfw, glad, imguilib, EnTT, Coral.Native, nfd, assimp
target_link_libraries(engine_core PUBLIC 
    yaml-cpp 
    glfw
    glad
    imguilib 
    EnTT 
    Coral.Native 
    nfd 
    assimp
)

# Apply common macro configurations and compiler flags
# (Redundant /bigobj flags removed - now in CompilerSettings.cmake)

# We apply optimizations and precompiled headers specific to core
if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_core)
endif()
