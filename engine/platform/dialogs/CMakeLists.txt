chained_add_engine_module(engine_platform_dialogs
    SOURCES
        file_dialogs.h
        file_dialogs.cpp
    DEPENDS
        nfd
        engine_core
)

target_include_directories(engine_platform_dialogs PRIVATE "${CMAKE_SOURCE_DIR}")
