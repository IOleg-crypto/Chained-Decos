add_library(core STATIC Component.cpp Component.h Entity.cpp Entity.h)

target_include_directories(core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core PUBLIC raylib)