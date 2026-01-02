add_library(project STATIC
    project.cpp
    project.h
)

target_include_directories(project PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(project PUBLIC
    core
    yaml-cpp
)

# Runtime is now in the root directory
