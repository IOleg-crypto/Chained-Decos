add_library(RuntimeCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_application.cpp
)

target_link_libraries(RuntimeCore PUBLIC engine)
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

target_link_libraries(ChainedRuntime PRIVATE RuntimeCore)
apply_engine_optimizations(ChainedRuntime)

install(TARGETS ChainedRuntime
    RUNTIME DESTINATION bin COMPONENT Runtime
)
