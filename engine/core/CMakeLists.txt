# Register Core sources to the unified engine target
target_sources(engine PRIVATE
    # Core
    ${CMAKE_CURRENT_SOURCE_DIR}/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uuid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uuid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/input.h
    ${CMAKE_CURRENT_SOURCE_DIR}/layer_stack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/layer_stack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/base.h
    ${CMAKE_CURRENT_SOURCE_DIR}/events.h
    ${CMAKE_CURRENT_SOURCE_DIR}/entry_point.h
    ${CMAKE_CURRENT_SOURCE_DIR}/layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/yaml.h
)
