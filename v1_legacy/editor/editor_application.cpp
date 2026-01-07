#include "editor_application.h"
#include "core/log.h"
#include "editor/editor_layer.h"
#include "editor/utils/editor_styles.h"
#include "engine/core/application/application.h"
#include "engine/core/engine_layer.h"
#include <imgui.h>


using namespace CHEngine;

EditorApplication::EditorApplication(int argc, char *argv[]) : Application("ChainedEditor")
{
    CD_INFO("[EditorApplication] Starting...");

    // 0. Push Engine Layer (Responsible for Simulation & Systems)
    PushLayer(new EngineLayer());

    // 1. Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // 2. Load Fonts
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Regular.ttf", 18.0f);

    static const ImWchar icons_ranges[] = {0xe005, 0xf8ff, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/fa-solid-900.ttf", 18.0f,
                                 &icons_config, icons_ranges);

    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Bold.ttf", 20.0f);
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Bold.ttf", 26.0f);

    // 3. Apply Professional Theme
    EditorStyles::ApplyDarkTheme();

    // 4. Push Editor Layer
    m_EditorLayer = new EditorLayer();
    PushLayer(m_EditorLayer);

    CD_INFO("[EditorApplication] Ready.");
}

EditorApplication::~EditorApplication()
{
}

void EditorApplication::OnEvent(CHEngine::Event &e)
{
    Application::OnEvent(e);
}

namespace CHEngine
{
Application *CreateApplication(int argc, char **argv)
{
    return new EditorApplication(argc, argv);
}
} // namespace CHEngine
