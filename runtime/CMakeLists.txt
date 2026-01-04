add_executable(ChainedRuntime
    runtime_main.cpp
    runtime_application.cpp
    runtime_application.h
)

target_link_libraries(ChainedRuntime PRIVATE engine)
target_include_directories(ChainedRuntime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Copy resources to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# Copy assets to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")
endif()
