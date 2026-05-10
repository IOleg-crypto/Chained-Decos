# NFD dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/include/nfd/CMakeLists.txt")
    set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/include/nfd" "${CMAKE_BINARY_DIR}/vendor/nfd" EXCLUDE_FROM_ALL)
    add_library(ChainedEngine::External::NFD ALIAS nfd)
endif()
