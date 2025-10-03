add_library(kernel STATIC
        Kernel.cpp
        Kernel.h
        IKernelService.h
        KernelServices.h
)

target_include_directories(kernel
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kernel PRIVATE raylib)


