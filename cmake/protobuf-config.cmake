# Minimal protobuf-config.cmake for in-tree build
# Provides what GNS's find_package(Protobuf CONFIG) expects

set(Protobuf_FOUND TRUE)
set(protobuf_FOUND TRUE)
set(Protobuf_VERSION "37.0.0")

# In-tree protobuf targets are already defined by add_subdirectory
# Just point to the right include dirs and libs
get_target_property(_proto_inc libprotobuf INTERFACE_INCLUDE_DIRECTORIES)
set(Protobuf_INCLUDE_DIRS "${_proto_inc}")
set(Protobuf_INCLUDE_DIR "${_proto_inc}")
set(Protobuf_LIBRARIES libprotobuf)
set(Protobuf_LIBRARY libprotobuf)

# Provide the protoc executable
if(TARGET protoc)
    set(Protobuf_PROTOC_EXECUTABLE "$<TARGET_FILE:protoc>")
endif()

# Ensure target aliases exist
if(NOT TARGET protobuf::libprotobuf)
    add_library(protobuf::libprotobuf ALIAS libprotobuf)
endif()
if(NOT TARGET protobuf::libprotobuf-lite)
    add_library(protobuf::libprotobuf-lite ALIAS libprotobuf-lite)
endif()
if(TARGET protoc AND NOT TARGET protobuf::protoc)
    add_executable(protobuf::protoc ALIAS protoc)
endif()

# protobuf v37 dropped the legacy protobuf_generate_cpp() helper, but GNS still
# calls it. Reimplement it on top of the modern protobuf_generate().
include("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/protobuf/cmake/protobuf-generate.cmake")

if(NOT COMMAND protobuf_generate_cpp)
    function(protobuf_generate_cpp SRCS HDRS)
        if(NOT ARGN)
            message(SEND_ERROR "protobuf_generate_cpp() called without any proto files")
            return()
        endif()

        set(_outdir "${CMAKE_CURRENT_BINARY_DIR}")
        set(_srcs "")
        set(_hdrs "")

        # GNS's .proto files import each other by bare filename, so each proto's own
        # directory has to be on the include path alongside the source root.
        set(_includes "")
        foreach(_proto ${ARGN})
            get_filename_component(_abs "${_proto}" ABSOLUTE)
            get_filename_component(_abs_dir "${_abs}" DIRECTORY)
            list(APPEND _includes "${_abs_dir}")
        endforeach()
        list(APPEND _includes "${CMAKE_CURRENT_SOURCE_DIR}")
        list(REMOVE_DUPLICATES _includes)

        set(_include_args "")
        foreach(_inc ${_includes})
            list(APPEND _include_args -I "${_inc}")
        endforeach()

        foreach(_proto ${ARGN})
            get_filename_component(_abs "${_proto}" ABSOLUTE)
            get_filename_component(_abs_dir "${_abs}" DIRECTORY)
            get_filename_component(_basename "${_proto}" NAME_WE)

            # protoc writes output relative to the include path that matched, which for
            # these bare imports is the proto's own directory — so the .pb.cc lands flat
            # in _outdir rather than mirroring the source tree.
            set(_gen_src "${_outdir}/${_basename}.pb.cc")
            set(_gen_hdr "${_outdir}/${_basename}.pb.h")

            add_custom_command(
                OUTPUT "${_gen_src}" "${_gen_hdr}"
                COMMAND protobuf::protoc
                ARGS --cpp_out "${_outdir}" ${_include_args} "${_abs}"
                DEPENDS "${_abs}" protobuf::protoc
                COMMENT "Running C++ protocol buffer compiler on ${_proto}"
                VERBATIM)

            list(APPEND _srcs "${_gen_src}")
            list(APPEND _hdrs "${_gen_hdr}")
        endforeach()

        set_source_files_properties(${_srcs} ${_hdrs} PROPERTIES GENERATED TRUE)
        set(${SRCS} ${_srcs} PARENT_SCOPE)
        set(${HDRS} ${_hdrs} PARENT_SCOPE)
    endfunction()
endif()
