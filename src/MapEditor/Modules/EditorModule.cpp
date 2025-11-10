#include "EditorModule.h"
#include "Engine/Kernel/Core/Kernel.h"
#include "Engine/Kernel/Core/KernelServices.h"
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

    // EditorModule doesn't create Editor - it's created by EditorApplication
    // This module can be used for editor-specific services if needed

    TraceLog(LOG_INFO, "[EditorModule] Initialized successfully");
    return true;
}

void EditorModule::Shutdown()
{
    TraceLog(LOG_INFO, "[EditorModule] Shutdown complete");
}

void EditorModule::Update(float deltaTime)
{
    // Editor update is handled by EditorApplication::OnPostUpdate()
    (void)deltaTime;
}

void EditorModule::Render()
{
    // Editor rendering is handled by EditorApplication::OnPostRender()
}

void EditorModule::RegisterServices(Kernel* kernel)
{
    if (!kernel) return;
    
    // Editor doesn't need additional services registered here
    // All editor functionality is handled directly in EditorApplication
}

std::vector<std::string> EditorModule::GetDependencies() const
{
    return {}; // No dependencies
}

