//
// ToolbarPanel.cpp - Main editor toolbar implementation
//

#include "ToolbarPanel.h"
#include "editor/IEditor.h"
#include <imgui.h>
#include <imgui_internal.h>

ToolbarPanel::ToolbarPanel(IEditor *editor) : m_editor(editor)
{
}

void ToolbarPanel::RenderToolButton(const char *label, Tool tool, const char *tooltip)
{
    bool isActive = (m_editor && m_editor->GetActiveTool() == tool);

    if (isActive)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }

    if (ImGui::Button(label, ImVec2(32, 28)))
    {
        if (m_editor)
        {
            m_editor->SetActiveTool(tool);
        }
    }

    if (isActive)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered() && tooltip)
    {
        ImGui::SetTooltip("%s", tooltip);
    }
}

void ToolbarPanel::RenderSeparator()
{
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
}

void ToolbarPanel::Render()
{
    if (!m_visible)
        return;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));

    if (ImGui::Begin("Toolbar", &m_visible, flags))
    {
        // File operations
        if (ImGui::Button("New", ImVec2(50, 28)))
        {
            if (m_editor)
                m_editor->ClearScene();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("New Scene (Ctrl+N)");

        ImGui::SameLine();

        if (ImGui::Button("Save", ImVec2(50, 28)))
        {
            if (m_editor)
                m_editor->SaveScene("");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Save Scene (Ctrl+S)");

        RenderSeparator();

        // Transform tools
        RenderToolButton("S", Tool::SELECT, "Select (Q)");
        ImGui::SameLine();
        RenderToolButton("M", Tool::MOVE, "Move (W)");
        ImGui::SameLine();
        RenderToolButton("R", Tool::ROTATE, "Rotate (E)");
        ImGui::SameLine();
        RenderToolButton("C", Tool::SCALE, "Scale (R)");

        RenderSeparator();

        // Create tools
        RenderToolButton("+", Tool::ADD_MODEL, "Place Object (T)");

        RenderSeparator();

        // Build Button
        if (ImGui::Button("B", ImVec2(32, 28)))
        {
            if (m_editor)
                m_editor->BuildGame();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Build Project (Compile)");

        RenderSeparator();

        // Play Mode Controls
        bool inPlayMode = m_editor && m_editor->IsInPlayMode();
        if (inPlayMode)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Stop", ImVec2(50, 28)))
            {
                m_editor->StopPlayMode();
            }
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
            if (ImGui::Button("Play", ImVec2(50, 28)))
            {
                m_editor->StartPlayMode();
            }
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(inPlayMode ? "Stop Simulation (Esc)" : "Start Simulation (Ctrl+P)");

        ImGui::SameLine();

        if (ImGui::Button("Run", ImVec2(50, 28)))
        {
            if (m_editor)
                m_editor->RunGame();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Run Standalone Game (Separate Window)");

        RenderSeparator();

        // Debug Toggles
        if (m_editor)
        {
            bool wire = m_editor->IsWireframeEnabled();
            if (wire)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            if (ImGui::Button("Wire", ImVec2(45, 28)))
            {
                m_editor->SetWireframeEnabled(!wire);
            }
            if (wire)
                ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle Wireframe Mode");

            ImGui::SameLine();

            bool coll = m_editor->IsCollisionDebugEnabled();
            if (coll)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
            if (ImGui::Button("Coll", ImVec2(45, 28)))
            {
                m_editor->SetCollisionDebugEnabled(!coll);
            }
            if (coll)
                ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle Collision Bounds Debug");

            ImGui::SameLine();

            bool uiMode = m_editor->IsUIDesignMode();
            if (uiMode)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.3f, 1.0f));
            if (ImGui::Button("UI", ImVec2(45, 28)))
            {
                m_editor->SetEditorMode(uiMode ? EditorMode::SCENE_3D : EditorMode::UI_DESIGN);
            }
            if (uiMode)
                ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle UI Design Mode");
        }
        // // Grid size
        // ImGui::Text("Grid:");
        // ImGui::SameLine();
        // int gridSize = m_editor ? m_editor->GetGridSize() : 50;
        // ImGui::SetNextItemWidth(80);
        // if (ImGui::SliderInt("##grid", &gridSize, 10, 200))
        // {
        //     if (m_editor)
        //         m_editor->SetGridSize(gridSize);
        // }
    }
    ImGui::End();

    ImGui::PopStyleVar();
}
