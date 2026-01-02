cmake_minimum_required(VERSION 3.20)

# Events library - independent event system
set(EVENTS_SOURCES
    event.h
    event.cpp
    key_event.h
    key_event.cpp
    mouse_event.h
    mouse_event.cpp
    application_event.h
    application_event.cpp
    menu_event.h
    ui_event_registry.h
)

add_library(events STATIC ${EVENTS_SOURCES})

target_include_directories(events PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/raylib/src
)

target_link_libraries(events PUBLIC
    raylib
)

set_target_properties(events PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
