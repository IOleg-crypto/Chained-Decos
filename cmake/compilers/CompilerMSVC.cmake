# MSVC Compiler Settings

# MSVC-specific settings
    add_compile_options(
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/DNDEBUG>
        /Zi /EHsc
        /MP                  # Multi-processor compilation
        /Zc:preprocessor     # Modern preprocessor
        /Gm-                 # Disable minimal rebuild (it's slower)
        /utf-8               # Use UTF-8 character set
        /bigobj              # Allow large object files (required for many modules)
        
        # Dead Code Elimination: Function-Level Linking
        $<$<CONFIG:Release>:/Gy>
    )

# Strip unused functions in Release
add_link_options($<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)

# /DEBUG:FULL — FASTLINK is deprecated in VS 2022 toolchain
add_link_options($<$<CONFIG:Debug>:/DEBUG:FULL>)

if(DISABLE_ALL_WARNINGS)
    add_compile_options(/W0)
elseif(ENABLE_WARNINGS)
    add_compile_options(/W4 /permissive-)
    if(WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(/W1)
endif()

if(ENABLE_SANITIZERS)
    add_compile_options(/fsanitize=address)
endif()

# Level 2 Security Hardening
# Note: /guard:cf on the LINKER pulls in uwapi.lib (Windows App Cert Kit),
# which is absent on the GitHub Actions windows-latest runner. The compile
# flag is sufficient for CFG code-gen; the linker flag is not needed here.
add_compile_options(/guard:cf /GS)
add_link_options(/DYNAMICBASE /NXCOMPAT)


    