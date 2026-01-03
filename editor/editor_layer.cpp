#include "editor_layer.h"
#include <imgui.h>
#include <rlImGui.h>

namespace CH
{
EditorLayer::EditorLayer() : Layer("EditorLayer")
{
}

void EditorLayer::OnAttach()
{
    rlImGuiSetup(true);
}

void EditorLayer::OnDetach()
{
    rlImGuiShutdown();
}

void EditorLayer::OnUpdate(float deltaTime)
{
}

void EditorLayer::OnImGuiRender()
{
    rlImGuiBegin();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            { /* TODO */
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Inspector");
    ImGui::End();

    rlImGuiEnd();
}

void EditorLayer::OnEvent(Event &e)
{
}
} // namespace CH
