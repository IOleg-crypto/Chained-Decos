# GLAD dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/glad/cmake/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/glad/cmake" "${CMAKE_BINARY_DIR}/vendor/glad" EXCLUDE_FROM_ALL)
    glad_add_library(engine_external_glad STATIC API gl:core=4.3)
else()
    message(FATAL_ERROR "GLAD submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/glad/cmake")
endif()
