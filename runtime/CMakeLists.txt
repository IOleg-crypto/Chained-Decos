add_library(RuntimeCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.cpp
)

target_link_libraries(RuntimeCore PUBLIC engine scripting)
target_include_directories(RuntimeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set PCH for RuntimeCore
target_precompile_headers(RuntimeCore PRIVATE 
    <string>
    <vector>
    <memory>
    <filesystem>
    "../engine/core/application.h"
)

# Generic Runtime Executable
add_executable(ChainedRuntime
    runtime_generic_entry.cpp
)

target_link_libraries(ChainedRuntime PRIVATE
    RuntimeCore
    engine
    scripting
    imguilib
)

# Copy engine resources to build output directory
add_custom_command(TARGET ChainedRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:ChainedRuntime>/resources"
    COMMENT "Copying engine resources..."
)

apply_engine_optimizations(ChainedRuntime)

install(TARGETS ChainedRuntime
    RUNTIME DESTINATION bin COMPONENT Runtime
)
