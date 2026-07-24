add_library(engine_core STATIC
    engine_module.h
    input.h
    input.cpp
    key_codes.h
    layer.h
    layer_stack.h
    layer_stack.cpp
    log.h
    mouse_codes.h
    platform.h
    platform.cpp
    profiler.h
    profiler.cpp
    service_locator.h
    window.h
    events/events.h
    events/input_events.h
    events/window_events.h
)

target_include_directories(engine_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_core PUBLIC
    engine_reflection
    yaml-cpp
    engine_external_entt
    glm
    engine_external_stb
    Coral.Native
    reflectcpp
    cereal
    spdlog
    libzstd_static
)

target_include_directories(engine_core PUBLIC
    "${CMAKE_SOURCE_DIR}/thirdparty/reflect-cpp/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/cereal/include"
    "${ZSTD_INCLUDE_DIR}"
)

if(TARGET engine_pch)
    target_link_libraries(engine_core PRIVATE engine_pch)
endif()
