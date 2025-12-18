set(EVENT_SOURCES
    Core/EventSystem.cpp
    Core/EventSystem.h
    Core/Event.h
    Core/EventDispatcher.h
    Core/EventDispatcher.cpp
    Events/GameEvents.h
)

add_library(event STATIC ${EVENT_SOURCES})

target_include_directories(event PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}
)

set_target_properties(event PROPERTIES POSITION_INDEPENDENT_CODE ON)