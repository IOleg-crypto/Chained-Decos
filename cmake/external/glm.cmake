# GLM dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/include/glm/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/include/glm" "${CMAKE_BINARY_DIR}/vendor/glm" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "glm submodule missing at ${CMAKE_SOURCE_DIR}/include/glm")
endif()
