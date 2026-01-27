# 1. Define the Runtime Core library (the reusable wrapper)
add_library(RuntimeCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_application.h
)
target_link_libraries(RuntimeCore PUBLIC engine)
target_include_directories(RuntimeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Apply Project-wide optimizations
apply_engine_optimizations(RuntimeCore)
