chained_add_engine_module(engine_platform_dialogs
    SOURCES
        dialogs.h
        dialogs.cpp
    DEPENDS
        engine_core
)

# PFD is a single-header library; keep its include path PRIVATE to avoid
# pulling <windows.h> (and the UUID typedef) into every translation unit.
target_include_directories(engine_platform_dialogs PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/thirdparty/portable-file-dialogs"
)
