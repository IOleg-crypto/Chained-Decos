#include "ImGuiLayer.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>


namespace ChainedDecos
{

ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
{
}

void ImGuiLayer::OnAttach()
{
    // Initialize ImGui with dark theme
    rlImGuiSetup(true);

    // Build fonts on attach
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (!io.Fonts->IsBuilt())
    {
        io.Fonts->Build();
    }
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
    // Block events if ImGui wants to capture input
    ImGuiIO &io = ImGui::GetIO();

    // Block mouse events if ImGui wants mouse
    if (e.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse)
    {
        e.Handled = true;
    }

    // Block keyboard events if ImGui wants keyboard
    if (e.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard)
    {
        e.Handled = true;
    }
}

void ImGuiLayer::Begin()
{
    rlImGuiBegin();
}

void ImGuiLayer::End()
{
    rlImGuiEnd();
}

} // namespace ChainedDecos
