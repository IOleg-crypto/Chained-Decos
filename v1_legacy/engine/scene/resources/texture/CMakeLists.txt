add_library(texture_resources STATIC
    texture_service.cpp
    texture_service.h
)

target_include_directories(texture_resources PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(texture_resources PUBLIC raylib)
