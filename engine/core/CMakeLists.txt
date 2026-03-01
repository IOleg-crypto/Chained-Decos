add_library(engine_core OBJECT
    layer_stack.cpp
    profiler.cpp
    uuid.cpp
    window.cpp
    input.cpp
    imgui_layer.cpp
    thread_pool.cpp
    application.cpp

    application.h
    imgui_layer.h
    base.h
    assert.h
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
    yaml.h
)

target_include_directories(engine_core PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
)

# Core module uses yaml-cpp for some configurations
target_link_libraries(engine_core PUBLIC yaml-cpp raylib imguilib EnTT Coral.Native)

# Apply common macro configurations and compiler flags
if(MSVC)
    target_compile_options(engine_core PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_core PRIVATE -Wa,-mbig-obj)
endif()

# We apply optimizations and precompiled headers specific to core
if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_core)
endif()
