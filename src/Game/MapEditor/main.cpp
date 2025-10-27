#include "Application.h"
#include "Editor/Editor.h"
#include "Engine/CameraController/CameraController.h"
#include "Engine/Model/Model.h"
#include <memory>

// Function to create and initialize the application with dependencies
std::unique_ptr<Application> CreateApplication()
{
    auto camera = std::make_shared<CameraController>();
    auto modelLoader = std::make_unique<ModelLoader>();
    auto editor = std::make_unique<Editor>(camera, std::move(modelLoader));
    return std::make_unique<Application>(1280, 720, std::move(editor));
}

// # --------
// # int main - init map editor
// # --------
int main()
{
    auto app = CreateApplication();
    app->Init();
    app->Run();
}