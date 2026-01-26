# Define the Runtime executable
add_executable(ChainedRuntime)

# Runtime Sources
target_sources(ChainedRuntime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_application.h
)

target_link_libraries(ChainedRuntime PRIVATE engine ChainedDecosGame)
target_include_directories(ChainedRuntime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Apply Project-wide optimizations
apply_engine_optimizations(ChainedRuntime)

# Project configuration for ChainedRuntime
set_target_properties(ChainedRuntime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy resources to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/bin")
endif()
