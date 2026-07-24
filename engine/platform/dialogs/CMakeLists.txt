add_library(engine_platform_dialogs STATIC
    dialogs.h
    dialogs.cpp
)

target_include_directories(engine_platform_dialogs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_platform_dialogs PUBLIC
    engine_core
)

# PFD is a single-header library; keep its include path PRIVATE to avoid
# pulling <windows.h> (and the UUID typedef) into every translation unit.
target_include_directories(engine_platform_dialogs PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/thirdparty/portable-file-dialogs"
)

if(TARGET engine_pch)
    target_link_libraries(engine_platform_dialogs PRIVATE engine_pch)
endif()
