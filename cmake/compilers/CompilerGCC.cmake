# GCC Compiler Settings

add_compile_options(
    $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-DNDEBUG>
    $<$<CONFIG:Release>:-ffunction-sections>
    $<$<CONFIG:Release>:-fdata-sections>
)

if(MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()

# Suppress overly strict C++23 template body checks for third-party headers (GLM)
add_compile_options(-Wno-template-body)

# Dead Code Elimination linkage and binary stripping for Release build
add_link_options(
    $<$<CONFIG:Release>:-Wl,--gc-sections>
)

if(DISABLE_ALL_WARNINGS)
    add_compile_options(-w)
elseif(ENABLE_WARNINGS)
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers -Wno-attributes)
    if(WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
else()
    add_compile_options(-Wno-all)
endif()

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

if(NOT WIN32)
    add_link_options(-Wl,-z,relro -Wl,-z,now)
endif()
