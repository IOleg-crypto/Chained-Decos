#include "imgui_layer.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

namespace CHEngine
{
ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
{
}

void ImGuiLayer::OnAttach()
{
    rlImGuiSetup(true);
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (!io.Fonts->IsBuilt())
        io.Fonts->Build();
}

void ImGuiLayer::OnDetach()
{
    rlImGuiShutdown();
}
void ImGuiLayer::OnUpdate(float deltaTime)
{
    m_Time += deltaTime;
}

void ImGuiLayer::OnEvent(Event &e)
{
    ImGuiIO &io = ImGui::GetIO();
    if (e.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse)
        e.Handled = true;
    if (e.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard)
        e.Handled = true;
}

void ImGuiLayer::Begin()
{
    rlImGuiBegin();
}
void ImGuiLayer::End()
{
    rlImGuiEnd();
}
} // namespace CHEngine
