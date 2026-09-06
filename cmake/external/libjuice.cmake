# libjuice — UDP Interactive Connectivity Establishment (ICE / STUN / TURN) library
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/libjuice")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(NO_TESTS ON CACHE BOOL "" FORCE)
    set(NO_SERVER ON CACHE BOOL "" FORCE)
    set(WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/libjuice" "${CMAKE_BINARY_DIR}/libjuice" EXCLUDE_FROM_ALL)

    message(STATUS "libjuice: configured (static lib, ICE/STUN/TURN)")
else()
    message(FATAL_ERROR "libjuice missing at ${CMAKE_SOURCE_DIR}/thirdparty/libjuice")
endif()
