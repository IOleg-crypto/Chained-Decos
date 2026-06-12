include(FetchContent)

FetchContent_Declare(
    bs_thread_pool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG        v4.1.0
    UPDATE_DISCONNECTED ON
)

FetchContent_MakeAvailable(bs_thread_pool)

if(NOT TARGET bs_thread_pool)
    add_library(bs_thread_pool INTERFACE)
    target_include_directories(bs_thread_pool INTERFACE "${bs_thread_pool_SOURCE_DIR}/include")
endif()

if(NOT TARGET ChainedEngine::External::BSThreadPool)
    add_library(ChainedEngine::External::BSThreadPool ALIAS bs_thread_pool)
endif()
