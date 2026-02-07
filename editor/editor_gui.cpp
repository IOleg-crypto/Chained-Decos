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

    bool EditorGUI::Property(const char *label, glm::vec2 &value, float speed, float min, float max)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
        bool changed = false;
        if (ImGui::DragFloat("##X", &value.x, speed, min, max, "X: %.2f")) changed = true;
        ImGui::PopItemWidth(); ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &value.y, speed, min, max, "Y: %.2f")) changed = true;
        ImGui::PopItemWidth();
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::Property(const char *label, glm::vec3 &value, float speed, float min, float max)
    {
        DrawPropertyLabel(label);
        ImGui::PushID(label);
        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        bool changed = false;
        if (ImGui::DragFloat("##X", &value.x, speed, min, max, "X: %.2f")) changed = true;
        ImGui::PopItemWidth(); ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &value.y, speed, min, max, "Y: %.2f")) changed = true;
        ImGui::PopItemWidth(); ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &value.z, speed, min, max, "Z: %.2f")) changed = true;
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

    bool EditorGUI::DrawVec3(const std::string &label, Vector3 &values, float resetValue)
    {
        bool changed = false;
        ImGui::PushID(label.c_str());
        DrawPropertyLabel(label.c_str());

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        DrawPropertyControl("##X", values.x, {0.8f, 0.1f, 0.15f, 1.0f}, "X", buttonSize, resetValue, changed); ImGui::SameLine();
        DrawPropertyControl("##Y", values.y, {0.2f, 0.7f, 0.2f, 1.0f}, "Y", buttonSize, resetValue, changed); ImGui::SameLine();
        DrawPropertyControl("##Z", values.z, {0.1f, 0.25f, 0.8f, 1.0f}, "Z", buttonSize, resetValue, changed);

        ImGui::PopStyleVar();
        ImGui::PopID();
        return changed;
    }

    bool EditorGUI::DrawVec3(const std::string &label, glm::vec3 &values, float resetValue)
    {
        Vector3 v = {values.x, values.y, values.z};
        if (DrawVec3(label, v, resetValue))
        {
            values.x = v.x;
            values.y = v.y;
            values.z = v.z;
            return true;
        }
        return false;
    }

    bool EditorGUI::DrawVec2(const std::string &label, Vector2 &values, float resetValue)
    {
        bool changed = false;
        ImGui::PushID(label.c_str());
        DrawPropertyLabel(label.c_str());

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

    bool EditorGUI::DrawVec2(const std::string &label, glm::vec2 &values, float resetValue)
    {
        Vector2 v = {values.x, values.y};
        if (DrawVec2(label, v, resetValue))
        {
            values.x = v.x;
            values.y = v.y;
            return true;
        }
        return false;
    }

    bool EditorGUI::ActionButton(const char* icon, const char* label)
    {
        std::string fullLabel = std::string(icon) + " " + label;
        return ImGui::Button(fullLabel.c_str());
    }

    void EditorGUI::ApplyTheme()
    {
        auto &style = ImGui::GetStyle();
        auto &colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4{0.08f, 0.085f, 0.09f, 1.0f};

        // Standard theme for headers/buttons/frames
        ImVec4 baseColor = {0.13f, 0.135f, 0.14f, 1.0f};
        ImVec4 hoverColor = {0.19f, 0.2f, 0.21f, 1.0f};
        ImVec4 activeColor = {0.15f, 0.1505f, 0.151f, 1.0f};

        colors[ImGuiCol_Header] = baseColor;
        colors[ImGuiCol_HeaderHovered] = hoverColor;
        colors[ImGuiCol_HeaderActive] = activeColor;

        colors[ImGuiCol_Button] = baseColor;
        colors[ImGuiCol_ButtonHovered] = hoverColor;
        colors[ImGuiCol_ButtonActive] = activeColor;

        colors[ImGuiCol_FrameBg] = baseColor;
        colors[ImGuiCol_FrameBgHovered] = hoverColor;
        colors[ImGuiCol_FrameBgActive] = activeColor;

        colors[ImGuiCol_Tab] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.25f, 0.26f, 0.27f, 1.0f};
        colors[ImGuiCol_TabActive] = ImVec4{0.18f, 0.19f, 0.2f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
        colors[ImGuiCol_TabUnfocusedActive] = baseColor;

        colors[ImGuiCol_TitleBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};
        colors[ImGuiCol_TitleBgActive] = colors[ImGuiCol_TitleBg];
        colors[ImGuiCol_TitleBgCollapsed] = colors[ImGuiCol_TitleBg];

        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
    }

    Camera3D EditorGUI::GetActiveCamera(SceneState state)
    {
        if (state == SceneState::Edit)
        {
            if (auto viewport = EditorLayer::Get().GetPanels().Get<ViewportPanel>())
                return viewport->GetCamera().GetRaylibCamera();
        }

        auto activeScene = EditorLayer::Get().GetActiveScene();
        if (activeScene)
        {
            // Find primary camera
            auto view = activeScene->GetRegistry().view<TransformComponent, CameraComponent>();
            
            CH_CORE_INFO("GetActiveCamera: Searching for primary camera, found {} cameras", view.size_hint());
            
            for (auto entity : view)
            {
                const auto& [tc, cc] = view.get<TransformComponent, CameraComponent>(entity);
                Entity e{entity, activeScene.get()};
                std::string entityName = e.HasComponent<TagComponent>() ? e.GetComponent<TagComponent>().Tag : "Unknown";
                
                CH_CORE_INFO("  Camera entity: '{}', Primary: {}, Position: [{}, {}, {}]", 
                    entityName, cc.Primary, tc.Translation.x, tc.Translation.y, tc.Translation.z);
                
                if (cc.Primary)
                {
                    Camera3D cam = { 0 };
                    cam.position = tc.Translation;
                    Matrix rotMat = QuaternionToMatrix(tc.RotationQuat);
                    Vector3 forward = Vector3Transform({0, 0, -1}, rotMat);
                    cam.target = Vector3Add(cam.position, forward);
                    cam.up = Vector3Transform({0, 1, 0}, rotMat);
                    
                    if (cc.Camera.GetProjectionType() == CHEngine::ProjectionType::Perspective)
                    {
                        cam.fovy = cc.Camera.GetPerspectiveVerticalFOV() * RAD2DEG;
                        cam.projection = CAMERA_PERSPECTIVE;
                    }
                    else
                    {
                        cam.fovy = cc.Camera.GetOrthographicSize();
                        cam.projection = CAMERA_ORTHOGRAPHIC;
                    }
                    
                    CH_CORE_INFO("  Using primary camera: '{}', FOV: {}", entityName, cam.fovy);
                    return cam;
                }
            }
        }
        
        // Fallback: default camera with warning
        CH_CORE_WARN("No primary camera found in scene! Add a Camera entity with CameraComponent.");
        return Camera3D{{10, 10, 10}, {0, 0, 0}, {0, 1, 0}, 45, CAMERA_PERSPECTIVE};
    }

    Ray EditorGUI::GetMouseRay(const Camera3D &camera, Vector2 localMousePos, Vector2 viewportSize)
    {
        // Calculate Normalized Device Coordinates (NDC)
        // Range: [-1, 1] for x, y, z
        // Viewport Top-Left is (-1, 1) in OpenGL NDC (Y-up)
        // ImGui/Local Mouse Top-Left is (0, 0) (Y-down)
        
        float ndc_x = (2.0f * localMousePos.x) / viewportSize.x - 1.0f;
        float ndc_y = 1.0f - (2.0f * localMousePos.y) / viewportSize.y;

        // Get View-Projection Matrix
        Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, viewportSize.x / viewportSize.y, 0.01f, 1000.0f);
        if (camera.projection == CAMERA_ORTHOGRAPHIC)
        {
             float aspect = viewportSize.x / viewportSize.y;
             float top = camera.fovy / 2.0f;
             float right = top * aspect;
             projection = MatrixOrtho(-right, right, -top, top, 0.01f, 1000.0f);
        }

        Matrix view = GetCameraMatrix(camera);

        // Calculate Ray using Raylib's math (to ensure matrix compatibility)
        // Note: Vector3Unproject uses MatrixMultiply(view, projection) internally and unprojects.
        // It expects source coordinates to be in [-1, 1] range (NDC) if viewport is not passed?
        // Wait, Raylib's Vector3Unproject implementation assumes source is NDC?
        // Let's verify standard Raylib GetMouseRay logic.
        // It does: ndc = ...; Unproject(ndc, proj, view);
        
        Vector3 nearPoint = Vector3Unproject({ndc_x, ndc_y, -1.0f}, projection, view);
        Vector3 farPoint = Vector3Unproject({ndc_x, ndc_y, 1.0f}, projection, view);

        Ray ray;
        ray.position = nearPoint;
        ray.direction = Vector3Normalize(Vector3Subtract(farPoint, nearPoint));

        return ray;
    }

} // namespace CHEngine
