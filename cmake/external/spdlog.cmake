# Configuration for spdlog 1.14.x
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)

# Use C++20 std::format for spdlog to avoid bundled fmt library issues with C++23/Clang
# Since the project already uses std::format successfully, this is the most robust fix.
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/spdlog/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/spdlog" "${CMAKE_BINARY_DIR}/vendor/spdlog" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "Spdlog submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/spdlog")
endif()

