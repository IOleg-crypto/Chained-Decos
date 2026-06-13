# yaml-cpp dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/include/yaml-cpp/CMakeLists.txt")
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    set(YAML_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    
    add_subdirectory("${CMAKE_SOURCE_DIR}/include/yaml-cpp" "${CMAKE_BINARY_DIR}/vendor/yaml-cpp" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "yaml-cpp submodule missing at ${CMAKE_SOURCE_DIR}/include/yaml-cpp")
endif()
