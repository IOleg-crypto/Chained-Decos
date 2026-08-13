# sodium — minimal bundled subset (XChaCha20-Poly1305 AEAD only)
# Extracted from yojimbo's vendored sodium. Provides encryption primitives
# for the networking layer.
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/sodium")
    add_library(sodium STATIC "${CMAKE_SOURCE_DIR}/thirdparty/sodium/sodium.c")
    target_include_directories(sodium PUBLIC "${CMAKE_SOURCE_DIR}/thirdparty/sodium")
    set_target_properties(sodium PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(sodium PRIVATE /w)
    else()
        target_compile_options(sodium PRIVATE -w)
    endif()

    message(STATUS "sodium: configured (minimal bundled subset)")
else()
    message(FATAL_ERROR "sodium missing at ${CMAKE_SOURCE_DIR}/thirdparty/sodium")
endif()
