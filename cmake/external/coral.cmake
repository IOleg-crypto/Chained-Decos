# Coral dependency (Scripting host)
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/coral/cmake/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/coral/cmake" "${CMAKE_BINARY_DIR}/vendor/coral" EXCLUDE_FROM_ALL)

    set_target_properties(Coral.Native PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF 
    )
    
    # MinGW fixes for Coral
    if(MINGW)
        set(CORAL_FIX_DIR "${CMAKE_BINARY_DIR}/vendor/coral_fixes")
        if(NOT EXISTS "${CORAL_FIX_DIR}")
            file(MAKE_DIRECTORY "${CORAL_FIX_DIR}")
        endif()
        file(WRITE "${CORAL_FIX_DIR}/ShlObj_core.h" "#pragma once\n#include <shlobj.h>\n")
        target_include_directories(Coral.Native PRIVATE "${CORAL_FIX_DIR}")
    endif()
else()
    message(FATAL_ERROR "Coral submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/coral/cmake")
endif()
