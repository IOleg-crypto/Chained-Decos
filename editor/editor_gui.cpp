#include "editor_gui.h"
#include "editor_events.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "editor/editor_layer.h"
#include "editor/panels/panel.h"
#include "editor/panels/viewport_panel.h"
#include "engine/core/application.h"
#include "engine/scene/project.h"
#include "engine/scene/components.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "raymath.h"
#include "nfd.h"
#include <filesystem>

namespace CHEngine
{
    // --- Internal Helpers ---
    
    static void DrawPropertyLabel(const char* label)
    {
        ImGui::Text(label);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.4f);
    }

    // --- Menu System Implementation ---

    void EditorGUI::DrawMenuBar(EditorPanels& panels)
    {
        if (!ImGui::BeginMenuBar()) return;

        // File Menu
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New Project", "Ctrl+Shift+N")) ProjectActions::New();
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project", "Ctrl+O")) ProjectActions::Open();
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project")) ProjectActions::Save();
            if (ImGui::MenuItem(ICON_FA_XMARK " Close Project")) Project::SetActive(nullptr);
            
            ImGui::Separator();
            
            if (ImGui::MenuItem(ICON_FA_FILE_CODE " New Scene", "Ctrl+N")) SceneActions::New();
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene", "Ctrl+S")) SceneActions::Save();
            if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save Scene As...", "Ctrl+Shift+S")) SceneActions::SaveAs();
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Scene", "Ctrl+L")) SceneActions::Open();
            
            ImGui::Separator();
            
            if (ImGui::MenuItem(ICON_FA_POWER_OFF " Exit")) Application::Get().Close();
            
            ImGui::EndMenu();
        }

        // View Menu
        if (ImGui::BeginMenu("View"))
        {
            // Toggle Panels
            panels.ForEach([](std::shared_ptr<Panel> panel) {
                if (panel->GetName() != "Viewport" && panel->GetName() != "Project Browser")
                    ImGui::MenuItem(panel->GetName().c_str(), nullptr, &panel->IsOpen());
            });

            ImGui::Separator();
            
            if (ImGui::MenuItem(ICON_FA_EXPAND " Fullscreen", "F11")) ToggleFullscreen();
            
            if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reset Layout")) {
                AppResetLayoutEvent e;
                Application::Get().OnEvent(e);
            }
            
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Current Layout as Default")) {
                AppSaveLayoutEvent e;
                Application::Get().OnEvent(e);
            }
            
            ImGui::EndMenu();
        }

        // Game Menu
        if (ImGui::BeginMenu("Game"))
        {
            if (ImGui::MenuItem(ICON_FA_GEARS " Project Settings")) {
                if (auto p = panels.Get("Project Settings")) p->IsOpen() = true;
            }
            
            if (ImGui::MenuItem(ICON_FA_ROCKET " Build & Run Standalone")) {
                AppLaunchRuntimeEvent e;
                Application::Get().OnEvent(e);
            }
            
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    PropertyBuilder EditorGUI::Begin()
    {
        return PropertyBuilder();
    }

    // --- Property Widgets Implementation ---

    bool EditorGUI::Property(const char *label, bool &value)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        bool changed = ImGui::Checkbox("##prop", &value);
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::Property(const char *label, float &value, float speed, float min, float max)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1);
        bool changed = ImGui::DragFloat("##prop", &value, speed, min, max);
        ImGui::PopItemWidth();
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::Property(const char *label, int &value, int min, int max)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1);
        bool changed = ImGui::DragInt("##prop", &value, 1.0f, min, max);
        ImGui::PopItemWidth();
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::Property(const char *label, std::string &value, bool multiline)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1);
        char buffer[256] = {0};
        value.copy(buffer, sizeof(buffer) - 1);
        bool changed = multiline ? 
            ImGui::InputTextMultiline("##prop", buffer, sizeof(buffer)) : 
            ImGui::InputText("##prop", buffer, sizeof(buffer));
        if (changed) value = buffer;
        ImGui::PopItemWidth();
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::Property(const char *label, Color &value)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1);
        float c[4] = {value.r/255.0f, value.g/255.0f, value.b/255.0f, value.a/255.0f};
        bool changed = ImGui::ColorEdit4("##prop", c);
        if (changed) value = {(unsigned char)(c[0]*255), (unsigned char)(c[1]*255), (unsigned char)(c[2]*255), (unsigned char)(c[3]*255)};
        ImGui::PopItemWidth();
        ImGui::PopID();
        return changed;
    }



    bool EditorGUI::Property(const char *label, Vector2 &value, float speed, float min, float max)
    {
        return DrawVec2(label, value, 0.0f);
    }

    bool EditorGUI::Property(const char *label, Vector3 &value, float speed, float min, float max)
    {
        return DrawVec3(label, value, 0.0f);
    }

    bool EditorGUI::Property(const char *label, int &value, const char **items, int itemCount)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        ImGui::PushItemWidth(-1);
        bool changed = ImGui::Combo("##prop", &value, items, itemCount);
        ImGui::PopItemWidth();
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::Property(const char *label, std::string &value, const std::string& filter)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        
        float width = ImGui::GetContentRegionAvail().x;
        float buttonSize = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        
        ImGui::PushItemWidth(width - buttonSize - 5.0f);
        
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
        
        bool changed = false;
        if (ImGui::InputText("##prop", buffer, sizeof(buffer)))
        {
            value = buffer;
            changed = true;
        }
        
        // Drag & Drop Target
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const char* path = (const char*)payload->Data;
                std::filesystem::path p = path;
                auto projectPath = Project::GetAssetDirectory();
                
                std::filesystem::path relativePath = std::filesystem::relative(p, projectPath);
                
                if (!relativePath.empty() && relativePath != ".")
                    value = relativePath.string();
                else
                    value = path; // Fallback
                
                changed = true;
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopItemWidth();
        ImGui::SameLine();
        
        if (ImGui::Button(ICON_FA_FOLDER_OPEN, {buttonSize, buttonSize}))
        {
            nfdu8char_t* outPath = NULL;
            nfdu8filteritem_t args[] = { { "Files", filter.c_str() } };
            
            nfdresult_t result;
            if (filter.empty())
                result = NFD_OpenDialogU8(&outPath, NULL, 0, NULL);
            else
                result = NFD_OpenDialogU8(&outPath, args, 1, NULL);
            
            if (result == NFD_OKAY)
            {
                std::filesystem::path p = outPath;
                auto projectPath = Project::GetAssetDirectory();
                std::filesystem::path relativePath = std::filesystem::relative(p, projectPath);
                
                if (!relativePath.empty() && relativePath != ".")
                    value = relativePath.string();
                else
                    value = outPath;
                    
                NFD_FreePathU8(outPath);
                changed = true;
            }
        }
        
        ImGui::PopID();
        return changed;
    }

    static void DrawPropertyControl(const char* id, float& val, ImVec4 color, const char* label, ImVec2 buttonSize, float resetValue, bool& changed)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {color.x * 1.1f, color.y * 1.1f, color.z * 1.1f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
        if (ImGui::Button(label, buttonSize)) { val = resetValue; changed = true; }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::DragFloat(id, &val, 0.1f, 0.0f, 0.0f, "%.2f")) changed = true;
        ImGui::PopItemWidth();
    }

    bool EditorGUI::DrawVec2(const char* label, Vector2 &values, float resetValue)
    {
        bool changed = false;
        ImGui::PushID(label);
        DrawPropertyLabel(label);

        ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        DrawPropertyControl("##X", values.x, {0.8f, 0.1f, 0.15f, 1.0f}, "X", buttonSize, resetValue, changed); ImGui::SameLine();
        DrawPropertyControl("##Y", values.y, {0.2f, 0.7f, 0.2f, 1.0f}, "Y", buttonSize, resetValue, changed);

        ImGui::PopStyleVar();
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::ActionButton(const char* icon, const char* label)
    {
        std::string fullLabel = std::string(icon) + " " + label;
        return ImGui::Button(fullLabel.c_str());
    }

    void EditorGUI::ApplyTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 12.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    bool EditorGUI::DrawVec3(const char* label, Vector3& values, float resetValue)
    {
        bool changed = false;
        ImGui::PushID(label);
        
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::Text(label);
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize)) 
        {
            values.x = resetValue;
            changed = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f")) changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize)) 
        {
            values.y = resetValue;
            changed = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f")) changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize)) 
        {
            values.z = resetValue;
            changed = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f")) changed = true;
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
        
        return changed;
    }

    Ray EditorGUI::GetMouseRay(const Camera3D &camera, const Vector2& localMousePos, const Vector2& viewportSize)
    {
        // NDC: [-1, 1], Y-up
        float ndc_x = (2.0f * localMousePos.x) / viewportSize.x - 1.0f;
        float ndc_y = 1.0f - (2.0f * localMousePos.y) / viewportSize.y;

        // View-Projection Matrix
        Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, viewportSize.x / viewportSize.y, 0.01f, 1000.0f);
        if (camera.projection == CAMERA_ORTHOGRAPHIC)
        {
             float aspect = viewportSize.x / viewportSize.y;
             float top = camera.fovy / 2.0f;
             float right = top * aspect;
             projection = MatrixOrtho(-right, right, -top, top, 0.01f, 1000.0f);
        }

        Matrix view = GetCameraMatrix(camera);
        Matrix viewProj = MatrixMultiply(view, projection);
        Matrix invViewProj = MatrixInvert(viewProj);
        
        // Unproject Near/Far points
        auto Unproject = [&](float x, float y, float z) -> Vector3 {
            float coords[4] = { x, y, z, 1.0f };
            float result[4] = { 0 };
            
            float resPoints[4] = {0};
            // Manual 4x4 multiplication for W component
            resPoints[0] = coords[0]*invViewProj.m0 + coords[1]*invViewProj.m4 + coords[2]*invViewProj.m8 + coords[3]*invViewProj.m12;
            resPoints[1] = coords[0]*invViewProj.m1 + coords[1]*invViewProj.m5 + coords[2]*invViewProj.m9 + coords[3]*invViewProj.m13;
            resPoints[2] = coords[0]*invViewProj.m2 + coords[1]*invViewProj.m6 + coords[2]*invViewProj.m10 + coords[3]*invViewProj.m14;
            resPoints[3] = coords[0]*invViewProj.m3 + coords[1]*invViewProj.m7 + coords[2]*invViewProj.m11 + coords[3]*invViewProj.m15;
            
            if (resPoints[3] == 0.0f) return {0,0,0};
            
            // Perspective division
            if (fabs(resPoints[3]) > 0.00001f)
                return { resPoints[0]/resPoints[3], resPoints[1]/resPoints[3], resPoints[2]/resPoints[3] };
            return {0,0,0};
        };

        Vector3 nearPoint = Unproject(ndc_x, ndc_y, -1.0f);
        Vector3 farPoint = Unproject(ndc_x, ndc_y, 1.0f);

        Ray ray;
        ray.position = nearPoint;
        
        // Manual vector math to avoid potential signature issues
        float dx = farPoint.x - nearPoint.x;
        float dy = farPoint.y - nearPoint.y;
        float dz = farPoint.z - nearPoint.z;
        float len = sqrtf(dx*dx + dy*dy + dz*dz);
        
        if (len > 0.0f)
        {
            ray.direction = { dx/len, dy/len, dz/len };
        }
        else
        {
            ray.direction = { 0, 0, -1 }; // Default forward?
        }

        return ray;
    }
}

