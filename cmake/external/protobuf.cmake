# ============================================================================
# Chained Engine - Protobuf Dependency (via FetchContent)
# ============================================================================
include(FetchContent)

set(PROTOBUF_VERSION "21.12") # Compatible with most GNS versions

# Options for Protobuf build
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "" FORCE)

# Fix for Clang on Windows: Protobuf incorrectly detects POSIX-like capabilities
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Protobuf: Applying Clang-on-Windows fix (disabling version scripts and forcing builtin atomics)")
    set(protobuf_HAVE_LD_VERSION_SCRIPT OFF CACHE BOOL "" FORCE)
    set(protobuf_HAVE_BUILTIN_ATOMICS ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    protobuf
    GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
    GIT_TAG        v${PROTOBUF_VERSION}
    GIT_SUBMODULES "" # Avoid heavy submodules if possible
)

message(STATUS "Protobuf: Fetching version ${PROTOBUF_VERSION}...")
FetchContent_MakeAvailable(protobuf)

# Ensure GNS can find the targets and variables it expects
if(TARGET libprotobuf)
    if(NOT TARGET protobuf::libprotobuf)
        add_library(protobuf::libprotobuf ALIAS libprotobuf)
    endif()
    
    # Set variables that FindProtobuf.cmake looks for
    set(Protobuf_INCLUDE_DIR "${protobuf_SOURCE_DIR}/src" CACHE PATH "Path to protobuf headers" FORCE)
    set(Protobuf_INCLUDE_DIRS "${Protobuf_INCLUDE_DIR}" CACHE PATH "Path to protobuf headers" FORCE)
    set(Protobuf_LIBRARY libprotobuf CACHE STRING "Protobuf library" FORCE)
    set(Protobuf_LIBRARIES libprotobuf CACHE STRING "Protobuf libraries" FORCE)
    set(Protobuf_FOUND TRUE CACHE BOOL "Protobuf found" FORCE)
    
    message(STATUS "Protobuf: Targets and variables initialized (Source: ${protobuf_SOURCE_DIR})")
endif()

if(TARGET protoc)
    if(NOT TARGET protobuf::protoc)
        add_executable(protobuf::protoc ALIAS protoc)
    endif()
    set(Protobuf_PROTOC_EXECUTABLE protoc CACHE STRING "Protoc executable" FORCE)
endif()
