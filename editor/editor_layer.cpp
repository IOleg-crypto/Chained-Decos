#include "editor_layer.h"
#include "engine/input.h"
#include "engine/renderer.h"
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

    m_EditorCamera.position = {10.0f, 10.0f, 10.0f};
    m_EditorCamera.target = {0.0f, 0.0f, 0.0f};
    m_EditorCamera.up = {0.0f, 1.0f, 0.0f};
    m_EditorCamera.fovy = 90.0f;
    m_EditorCamera.projection = CAMERA_PERSPECTIVE;
}

void EditorLayer::OnDetach()
{
    rlImGuiShutdown();
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        UpdateCamera(&m_EditorCamera, CAMERA_FREE);
    }
}

void EditorLayer::OnRender()
{
    Renderer::BeginScene(m_EditorCamera);
    Renderer::DrawGrid(20, 1.0f);
    Renderer::EndScene();
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
