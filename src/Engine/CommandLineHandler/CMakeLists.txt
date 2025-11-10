add_library(cmdhandler STATIC
    Core/CommandLineHandler.cpp
    Core/CommandLineHandler.h
)

target_include_directories(cmdhandler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(cmdhandler PUBLIC
    raylib)

set_target_properties(cmdhandler PROPERTIES POSITION_INDEPENDENT_CODE ON)
