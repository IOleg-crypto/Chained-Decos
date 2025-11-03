#include "EditorModule.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "MapEditor/Editor/Editor.h"
#include "Engine/CameraController/CameraController.h"
#include "Engine/Model/Model.h"
#include <raylib.h>

EditorModule::EditorModule()
{
}

bool EditorModule::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[EditorModule] Kernel is null");
        return false;
    }

    // Створюємо залежності для Editor
    m_cameraController = std::make_shared<CameraController>();
    m_modelLoader = std::make_unique<ModelLoader>();

    // Створюємо Editor
    m_editor = std::make_unique<Editor>(m_cameraController, std::move(m_modelLoader));

    TraceLog(LOG_INFO, "[EditorModule] Initialized successfully");
    return true;
}

void EditorModule::Shutdown()
{
    if (m_editor) {
        m_editor.reset();
    }
    if (m_cameraController) {
        m_cameraController.reset();
    }
    if (m_modelLoader) {
        m_modelLoader.reset();
    }
    
    TraceLog(LOG_INFO, "[EditorModule] Shutdown complete");
}

void EditorModule::Update(float deltaTime)
{
    if (m_editor) {
        m_editor->Update();
        m_editor->HandleInput();
    }
}

void EditorModule::Render()
{
    if (m_editor) {
        m_editor->Render();
        m_editor->RenderImGui();
    }
}

void EditorModule::RegisterServices(Kernel* kernel)
{
    if (!kernel) return;
    
    // Editor може бути зареєстрований як сервіс, якщо потрібно
}

std::vector<std::string> EditorModule::GetDependencies() const
{
    return {}; // No dependencies
}

