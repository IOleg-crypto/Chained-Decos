#include "EditorModule.h"
#include "core/engine/Engine.h"
#include <raylib.h>

EditorModule::EditorModule()
{
}

bool EditorModule::Initialize(Engine *engine)
{
    if (!engine)
    {
        TraceLog(LOG_ERROR, "[EditorModule] Engine is null");
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

void EditorModule::RegisterServices(Engine *engine)
{
    if (!engine)
        return;

    // Editor doesn't need additional services registered here
    // All editor functionality is handled directly in EditorApplication
}

std::vector<std::string> EditorModule::GetDependencies() const
{
    return {}; // No dependencies
}
