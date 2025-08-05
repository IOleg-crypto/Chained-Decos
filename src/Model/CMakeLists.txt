add_library(model STATIC
    Model.cpp
    Model.h
    ModelInstance.cpp
    ModelInstance.h
)
target_link_libraries(model PUBLIC raylib nlohmann_json collision)