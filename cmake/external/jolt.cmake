# Chained Engine - Jolt Physics Dependency

include(FetchContent)

FetchContent_Declare(
    jolt
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.0.0
    SOURCE_SUBDIR  Build
    UPDATE_DISCONNECTED ON
)

set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)
set(USE_AVX2 ON CACHE BOOL "" FORCE)
set(USE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(jolt)

# Jolt doesn't provide a clean target for some configurations, 
# so we might need to manually link if it fails.
if(TARGET Jolt)
    add_library(JoltPhysics::Jolt ALIAS Jolt)
endif()
