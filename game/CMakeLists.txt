add_executable(ChainedDecos
    src/main.cpp
)

target_link_libraries(ChainedDecos PRIVATE engine editor)
target_include_directories(ChainedDecos PRIVATE src)

# Copy resources to build directory
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")
endif()
