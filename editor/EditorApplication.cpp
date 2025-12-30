#include "EditorApplication.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/ServiceRegistry.h"
#include "editor/EditorLayer.h"
#include "runtime/RuntimeLayer.h"
#include "scene/main/LevelManager.h"

#include "editor/utils/EditorStyles.h"
#include <imgui.h>
#include <rlImGui.h>

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

    // Note: Player system now uses ECS components (PlayerComponent)
    // No need for IPlayer service registration
}

void EditorApplication::OnStart()
{
    CD_INFO("[EditorApplication] Starting...");

    // 1. Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // 2. Load Fonts (Hazel style)
    io.Fonts->Clear();

    // Regular text (18.0f) - DEFAULT FONT
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Regular.ttf", 18.0f);

    // Add Icons to the first font (Regular)
    static const ImWchar icons_ranges[] = {0xe005, 0xf8ff, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/fa-solid-900.ttf", 18.0f,
                                 &icons_config, icons_ranges);

    // Bold / Header text (20.0f)
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Bold.ttf", 20.0f);

    // Title text (26.0f)
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Bold.ttf", 26.0f);

    // 3. Apply Professional Theme
    EditorStyles::ApplyDarkTheme();

    // 4. Push Editor Layer
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
