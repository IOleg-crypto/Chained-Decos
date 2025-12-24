add_library(texture_resources STATIC
    TextureService.cpp
    TextureService.h
)

target_include_directories(texture_resources PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(texture_resources PUBLIC raylib)
