add_library(gui STATIC
    Helpers/CGUIHelpers.cpp
    Helpers/CGUIHelpers.h
)

target_include_directories(gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(gui PUBLIC
    imguilib
)

set_target_properties(gui PROPERTIES POSITION_INDEPENDENT_CODE ON)
