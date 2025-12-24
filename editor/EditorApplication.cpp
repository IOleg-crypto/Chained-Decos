#include "EditorApplication.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/ServiceRegistry.h"
#include "core/application/EntryPoint.h"
#include "editor/EditorLayer.h"
#include "project/ChainedDecos/player/core/Player.h"
#include "scene/main/core/LevelManager.h"

#include <imgui.h>
#include <rlImGui.h>

DECLARE_APPLICATION(EditorApplication)

using namespace CHEngine;

EditorApplication::EditorApplication(int argc, char *argv[])
{
}

EditorApplication::~EditorApplication()
{
}

void EditorApplication::OnConfigure(EngineConfig &config)
{
    config.windowName = "ChainedEditor";
    config.width = 1600;
    config.height = 900;
}

void EditorApplication::OnRegister()
{
    Engine::Instance().RegisterModule(std::make_unique<LevelManager>());

    // Register a dummy Player service to resolve engine errors
    // Use the concrete Player class as it implements IPlayer
    auto dummyPlayer = std::make_shared<Player>(nullptr);
    ServiceRegistry::Register<IPlayer>(dummyPlayer);
}

void EditorApplication::OnStart()
{
    CD_INFO("[EditorApplication] Starting...");

    // 1. Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // 2. Load Fonts (Hazel style, usually in Layer but let's keep it here for now)
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Regular.ttf", 14.0f);

    // Add Icons
    static const ImWchar icons_ranges[] = {0xe005, 0xf8ff, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/fa-solid-900.ttf", 14.0f,
                                 &icons_config, icons_ranges);

    // 3. Push Editor Layer
    m_EditorLayer = new EditorLayer();
    if (GetAppRunner())
    {
        GetAppRunner()->PushLayer(m_EditorLayer);
    }

    CD_INFO("[EditorApplication] Ready.");
}

void EditorApplication::OnUpdate(float deltaTime)
{
}

void EditorApplication::OnRender()
{
}

void EditorApplication::OnImGuiRender()
{
    if (m_EditorLayer)
        m_EditorLayer->OnImGuiRender();
}

void EditorApplication::OnShutdown()
{
}

void EditorApplication::OnEvent(CHEngine::Event &e)
{
}
