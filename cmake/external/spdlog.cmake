include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    UPDATE_DISCONNECTED ON
)

# Configuration for spdlog 1.14.x
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)

# Use C++20 std::format for spdlog to avoid bundled fmt library issues with C++23/Clang
# Since the project already uses std::format successfully, this is the most robust fix.
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(spdlog)

if(NOT TARGET spdlog)
    # Alias the underlying target directly
    if(TARGET spdlog)
    else()
    endif()
endif()
