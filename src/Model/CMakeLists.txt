add_library(model STATIC
    Model.cpp
    Model.h
    ModelInstance.cpp
    ModelInstance.h
)
target_include_directories(model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Model)
target_link_libraries(model PUBLIC raylib nlohmann_json) 