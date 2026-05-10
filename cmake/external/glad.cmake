# GLAD dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/include/glad/cmake/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/include/glad/cmake" "${CMAKE_BINARY_DIR}/vendor/glad" EXCLUDE_FROM_ALL)
    glad_add_library(engine_external_glad STATIC API gl:core=4.3)
    add_library(ChainedEngine::External::GLAD ALIAS engine_external_glad)
else()
    message(FATAL_ERROR "GLAD submodule missing at ${CMAKE_SOURCE_DIR}/include/glad/cmake")
endif()
