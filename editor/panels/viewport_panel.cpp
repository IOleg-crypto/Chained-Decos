#include "viewport_panel.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/graphics/draw_command.h"
#include "engine/scene/scene.h"
#include "imgui.h"
#include "raylib.h"
#include "rlimgui.h"
#include "ui/editor_gui.h"

namespace CHEngine
{
static void ClearSceneBackground(Scene *scene, Vector2 size)
{
    auto mode = scene->GetBackgroundMode();
    if (mode == BackgroundMode::Color)
    {
        ClearBackground(scene->GetBackgroundColor());
    }
    else if (mode == BackgroundMode::Texture)
    {
        auto &path = scene->GetBackgroundTexturePath();
        if (!path.empty())
        {
            // Fallback for now
            ClearBackground(scene->GetBackgroundColor());
        }
    }
    else if (mode == BackgroundMode::Environment3D)
    {
        ClearBackground(BLACK);
    }
}

ViewportPanel::ViewportPanel()
{
    m_Name = "Viewport";
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    m_ViewportTexture = LoadRenderTexture(w > 0 ? w : 1280, h > 0 ? h : 720);
}

ViewportPanel::~ViewportPanel()
{
    UnloadRenderTexture(m_ViewportTexture);
}

void ViewportPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin(m_Name.c_str(), &m_IsOpen);
    ImGui::PushID(this);

    m_Gizmo.RenderAndHandle(m_CurrentTool);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x != m_ViewportTexture.texture.width ||
        viewportSize.y != m_ViewportTexture.texture.height)
    {
        if (viewportSize.x > 0 && viewportSize.y > 0)
        {
            UnloadRenderTexture(m_ViewportTexture);
            m_ViewportTexture = LoadRenderTexture((int)viewportSize.x, (int)viewportSize.y);
            EditorLayer::Get().s_ViewportSize = viewportSize;
        }
    }

    auto activeScene = Application::Get().GetActiveScene();
    if (!activeScene)
    {
        ImGui::PopID();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    // Render to texture
    BeginTextureMode(m_ViewportTexture);
    ClearSceneBackground(activeScene.get(), {viewportSize.x, viewportSize.y});

    Camera3D camera = EditorUI::GUI::GetActiveCamera(EditorLayer::Get().GetSceneState());
    activeScene->OnRender(camera, &EditorLayer::Get().GetDebugRenderFlags());

    EndTextureMode();

    rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);

    ImGui::PopID();
    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::OnUpdate(float deltaTime)
{
    m_EditorCamera.OnUpdate(deltaTime);
}

void ViewportPanel::OnEvent(Event &e)
{
    // m_EditorCamera doesn't have OnEvent yet, so we just let it be for now
}

} // namespace CHEngine
