add_library(kernel STATIC
        Core/Kernel.cpp
        Core/Kernel.h
        Core/KernelServices.h
        Interfaces/IKernelService.h
)

target_include_directories(kernel
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kernel PRIVATE raylib)

set_target_properties(kernel PROPERTIES POSITION_INDEPENDENT_CODE ON)


