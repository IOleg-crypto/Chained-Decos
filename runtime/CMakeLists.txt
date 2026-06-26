add_library(RuntimeCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.cpp
)

target_link_libraries(RuntimeCore PUBLIC ChainedEngine::Framework)
target_include_directories(RuntimeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set PCH for RuntimeCore
target_precompile_headers(RuntimeCore PRIVATE 
    <string>
    <vector>
    <memory>
    <filesystem>
    "${CMAKE_SOURCE_DIR}/engine/app/application.h"
)

# Generic Runtime Executable
add_executable(ChainedRuntime
    runtime_generic_entry.cpp
)

target_link_libraries(ChainedRuntime PRIVATE
    RuntimeCore
    ChainedEngine::Framework
    ChainedDecos
    engine_core
    engine_graphics
    engine_audio
    engine_physics
    engine_imgui
    engine_scripting
)

apply_engine_optimizations(ChainedRuntime)

install(TARGETS ChainedRuntime
    RUNTIME DESTINATION bin COMPONENT Runtime
)
