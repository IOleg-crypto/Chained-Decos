set(REFLECTCPP_YAML ON CACHE BOOL "" FORCE)
set(REFLECTCPP_JSON ON CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_SOURCE_DIR}/include/reflect-cpp/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/include/reflect-cpp" EXCLUDE_FROM_ALL)
    add_library(ChainedEngine::External::reflect-cpp ALIAS reflectcpp)
else()
    message(FATAL_ERROR "reflect-cpp submodule missing at ${CMAKE_SOURCE_DIR}/include/reflect-cpp")
endif()
