add_library(kernel INTERFACE)

target_include_directories(kernel
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kernel INTERFACE raylib)


