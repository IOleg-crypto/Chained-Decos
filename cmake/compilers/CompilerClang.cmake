# Clang Compiler Settings (includes AppleClang and clang-cl)

if(MSVC)
    # clang-cl behaves like MSVC
    add_compile_options(/Zc:preprocessor /utf-8 /bigobj)
else()
    add_compile_options(
        $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-DNDEBUG>
        $<$<CONFIG:Release>:-ffunction-sections>
        $<$<CONFIG:Release>:-fdata-sections>
    )

    # -mstackrealign: MinGW/ELF Clang at -O0 may misalign the stack,
    # causing SSE/AVX faults inside CoreCLR/hostfxr (same as GCC).
    if(MINGW OR (NOT WIN32))
        add_compile_options(
            $<$<CONFIG:Debug>:-mstackrealign>
        )
    endif()

    if(MINGW)
        add_compile_options(-Wa,-mbig-obj)
    endif()

    # Dead Code Elimination linkage
    if(MINGW OR (NOT WIN32))
        # ELF / MinGW ld — standard gc-sections
        add_link_options(
            $<$<CONFIG:Release>:-Wl,--gc-sections>
        )
    endif()

    # Windows targeting lld-link — /OPT:REF /OPT:ICF replaces --gc-sections
    if(WIN32 AND NOT MINGW)
        add_link_options(
            $<$<CONFIG:Release>:/OPT:REF>
            $<$<CONFIG:Release>:/OPT:ICF>
        )
    endif()

    if(DISABLE_ALL_WARNINGS)
        add_compile_options(-w)
    elseif(ENABLE_WARNINGS)
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wmost -Wno-missing-braces -Wno-missing-field-initializers -Wno-attributes)
        if(WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    else()
        add_compile_options(-Wno-all)
    endif()
endif()

# Prefer LLD linker (same logic as CompilerGCC.cmake).
# Use the name "lld" (not the full path) so Clang resolves the correct
# sub-driver (ld.lld for ELF/MinGW, lld-link for MSVC ABI).
find_program(CH_CLANG_LLD_LINKER NAMES ld.lld lld lld-link)
if(CH_CLANG_LLD_LINKER)
    add_link_options(-fuse-ld=lld)
    message(STATUS "Clang: using LLD linker (${CH_CLANG_LLD_LINKER})")
endif()

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

if(NOT WIN32)
    add_link_options(-Wl,-z,relro -Wl,-z,now)
endif()
