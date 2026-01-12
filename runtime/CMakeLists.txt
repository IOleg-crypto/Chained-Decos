set(RUNTIME_SOURCES
    runtime_main.cpp
    runtime_application.cpp
    runtime_application.h
)

add_executable(ChainedRuntime ${RUNTIME_SOURCES})

target_link_libraries(ChainedRuntime PRIVATE engine)
target_include_directories(ChainedRuntime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Project configuration for ChainedRuntime
set_target_properties(ChainedRuntime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy resources and potentially assets to build directory if they exist at root
# Note: Assets usually stay in project directories now, but generic resources might be needed.
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}/bin")
endif()
