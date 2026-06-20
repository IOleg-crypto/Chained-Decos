# NFD dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/nfd/CMakeLists.txt")
    set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/nfd" "${CMAKE_BINARY_DIR}/vendor/nfd" EXCLUDE_FROM_ALL)
endif()
