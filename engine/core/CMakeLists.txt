add_library(engine_core ${SUBMODULE_LIB_TYPE}
        layer_stack.cpp
        profiler.cpp
        uuid.cpp
        input.cpp
        imgui_layer.cpp
        thread_pool.cpp
        service_locator.cpp
        application.h
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
        engine_service.h
        ch_math.h
        key_codes.h
        mouse_codes.h
        reflection.h
        yaml_conversions.h
        ../platform/utils/dialogs.cpp
        ../platform/utils/dialogs.h
        ../platform/glfw/glfw_window.cpp
        ../platform/glfw/glfw_window.h
)

target_include_directories(engine_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_core 
PUBLIC 
    ChainedEngine::External::YamlCpp
    ChainedEngine::External::EnTT
    ChainedEngine::External::GLM
    ChainedEngine::External::STB
    ChainedEngine::External::GLFW
    ChainedEngine::External::GLAD
    ChainedEngine::External::ImGui
    ChainedEngine::External::Miniaudio
    ChainedEngine::External::Coral
    ChainedEngine::External::NFD
)

# We apply optimizations and precompiled headers specific to core
if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_core)
endif()
