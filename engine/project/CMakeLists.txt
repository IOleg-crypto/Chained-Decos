add_library(engine_project STATIC
    project.h
    project.cpp
)

target_include_directories(engine_project PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_project PUBLIC
    engine_core
)

if(TARGET engine_pch)
    target_link_libraries(engine_project PRIVATE engine_pch)
endif()
