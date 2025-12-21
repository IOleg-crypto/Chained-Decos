add_library(font_resources STATIC
    FontService.cpp
    FontService.h
)

target_include_directories(font_resources PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(font_resources PUBLIC raylib)
