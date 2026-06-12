chained_add_engine_module(engine_core
    SOURCES
        layer_stack.cpp
        profiler.cpp
        input.cpp
        imgui_layer.cpp

        application.h
        platform.h
        platform.cpp
        imgui_layer.h
        entry_point.h
        events.h
        input.h
        layer.h
        layer_stack.h
        log.h
        profiler.h
        window.h

        key_codes.h
        mouse_codes.h
        ../platform/glfw/glfw_window.cpp
        ../platform/glfw/glfw_window.h
    DEPENDS
        engine_reflection
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
        ChainedEngine::External::reflect-cpp
        ChainedEngine::External::Cereal
        ChainedEngine::External::spdlog
        ChainedEngine::External::BSThreadPool
        zstd::libzstd_static
)

target_include_directories(engine_core PUBLIC 
    "${CMAKE_SOURCE_DIR}/include/reflect-cpp/include"
    "${CMAKE_SOURCE_DIR}/include/cereal/include"
    "${ZSTD_INCLUDE_DIR}"
)
