#define GLFW_INCLUDE_NONE
#include "editor_gui.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/scene/components.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "panels/panel.h"
#include "panels/viewport_panel.h"
#include "raymath.h"
#include "rlImGui.h"
#include "engine/scene/components/control_component.h"

namespace CHEngine::EditorUI
{
    void GUI::BeginProperties(float columnWidth)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
    }

    void GUI::EndProperties()
    {
        ImGui::Columns(1);
        ImGui::PopStyleVar();
    }

    bool GUI::DrawVec3Control(const std::string &label, Vector3 &values, float resetValue, float columnWidth)
    {
        bool changed = false;

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
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
        if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
            changed = true;
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
        if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
            changed = true;
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
        if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();

        return changed;
    }

    bool GUI::DrawVec2Control(const std::string &label, glm::vec2 &values, float resetValue, float columnWidth)
    {
        bool changed = false;
        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
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
        if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
            changed = true;
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
        if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();

        return changed;
    }
    

    bool GUI::Property(const char *label, bool &value)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);
        if (ImGui::Checkbox("##prop", &value))
            changed = true;
        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, float &value, float speed, float min, float max)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);
        if (ImGui::DragFloat("##prop", &value, speed, min, max))
            changed = true;
        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, Vector2 &value, float speed, float min, float max)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
        if (ImGui::DragFloat("##X", &value.x, speed, min, max, "X: %.2f"))
            changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &value.y, speed, min, max, "Y: %.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, Vector3 &value, float speed, float min, float max)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        if (ImGui::DragFloat("##X", &value.x, speed, min, max, "X: %.2f"))
            changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &value.y, speed, min, max, "Y: %.2f"))
            changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &value.z, speed, min, max, "Z: %.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, glm::vec2 &value, float speed, float min, float max)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
        if (ImGui::DragFloat("##X", &value.x, speed, min, max, "X: %.2f"))
            changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &value.y, speed, min, max, "Y: %.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, glm::vec3 &value, float speed, float min, float max)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        if (ImGui::DragFloat("##X", &value.x, speed, min, max, "X: %.2f"))
            changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &value.y, speed, min, max, "Y: %.2f"))
            changed = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &value.z, speed, min, max, "Z: %.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, std::string &value, bool multiline)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        if (!value.empty())
        {
            size_t len = value.copy(buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';
        }

        if (multiline)
        {
            if (ImGui::InputTextMultiline("##prop", buffer, sizeof(buffer)))
            {
                value = std::string(buffer);
                changed = true;
            }
        }
        else
        {
            if (ImGui::InputText("##prop", buffer, sizeof(buffer)))
            {
                value = std::string(buffer);
                changed = true;
            }
        }

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, Color &value)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        float c[4] = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
        if (ImGui::ColorEdit4("##prop", c))
        {
            value.r = (unsigned char)(c[0] * 255.0f);
            value.g = (unsigned char)(c[1] * 255.0f);
            value.b = (unsigned char)(c[2] * 255.0f);
            value.a = (unsigned char)(c[3] * 255.0f);
            changed = true;
        }

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, int &value)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        if (ImGui::DragInt("##prop", &value))
        {
            changed = true;
        }

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    bool GUI::Property(const char *label, int &value, const char **items, int itemCount)
    {
        bool changed = false;
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushID(label);

        if (ImGui::Combo("##prop", &value, items, itemCount))
        {
            changed = true;
        }

        ImGui::PopID();
        ImGui::NextColumn();
        return changed;
    }

    Camera3D GUI::GetActiveCamera(SceneState state)
    {
        if (state == SceneState::Edit)
        {
            if (auto viewport = EditorLayer::Get().GetPanels().Get<ViewportPanel>())
                return viewport->GetCamera().GetRaylibCamera();
        }

        auto activeScene = Application::Get().GetActiveScene();
        if (activeScene)
            return activeScene->GetActiveCamera();

        // Default fallback
        Camera3D camera = {0};
        camera.position = {10.0f, 10.0f, 10.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }

    void GUI::CalculatePlayCamera(Camera3D &camera, Scene *scene)
    {
    }


    void GUI::SetDarkThemeColors()
    {
        auto &colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{0.08f, 0.085f, 0.09f, 1.0f};

        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.13f, 0.135f, 0.14f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.19f, 0.2f, 0.21f, 1.0f};
        colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.13f, 0.135f, 0.14f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = ImVec4{0.19f, 0.2f, 0.21f, 1.0f};
        colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{0.13f, 0.135f, 0.14f, 1.0f};
        colors[ImGuiCol_FrameBgHovered] = ImVec4{0.19f, 0.2f, 0.21f, 1.0f};
        colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.25f, 0.26f, 0.27f, 1.0f};
        colors[ImGuiCol_TabActive] = ImVec4{0.18f, 0.19f, 0.2f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.13f, 0.135f, 0.14f, 1.0f};

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

        // Styling tweaks
        auto& style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
    }
    static void MenuFile(const EventCallbackFn &callback)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New Project", "Ctrl+Shift+N"))
                ProjectActions::Open();
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project", "Ctrl+O"))
                ProjectActions::Open();
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project"))
                ProjectActions::Save();
            if (ImGui::MenuItem(ICON_FA_XMARK " Close Project"))
                Project::SetActive(nullptr);

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_FILE_CODE " New Scene", "Ctrl+N"))
                SceneActions::New();
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene", "Ctrl+S"))
                SceneActions::Save();
            if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save Scene As...", "Ctrl+Shift+S"))
                SceneActions::SaveAs();
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Scene", "Ctrl+L"))
                SceneActions::Open();

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_POWER_OFF " Exit"))
                Application::Get().Close();

            ImGui::EndMenu();
        }
    }

    static void MenuEdit(const EventCallbackFn &callback)
    {
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem(ICON_FA_ROTATE_LEFT " Undo", "Ctrl+Z"))
            {
            }
            if (ImGui::MenuItem(ICON_FA_ROTATE_RIGHT " Redo", "Ctrl+Y"))
            {
            }

            ImGui::Separator();

            ImGui::EndMenu();
        }
    }

    static void MenuView(const MenuBarState &state, const EventCallbackFn &callback)
    {
        if (ImGui::BeginMenu("View"))
        {
            if (state.Panels)
            {
                for (auto &panel : *state.Panels)
                {
                    if (panel->GetName() == "Viewport" || panel->GetName() == "Project Browser")
                        continue;

                    ImGui::MenuItem(panel->GetName().c_str(), nullptr, &panel->IsOpen());
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_EXPAND " Fullscreen", "F11", IsWindowFullscreen()))
                ToggleFullscreen();

            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reset Layout"))
            {
                AppResetLayoutEvent e;
                callback(e);
            }

            ImGui::EndMenu();
        }
    }

    static void MenuGame(const MenuBarState &state, const EventCallbackFn &callback)
    {
        if (ImGui::BeginMenu("Game"))
        {
            if (ImGui::MenuItem(ICON_FA_GEARS " Project Settings"))
            {
                if (state.Panels)
                {
                    for (auto &panel : *state.Panels)
                    {
                        if (panel->GetName() == "Project Settings")
                            panel->IsOpen() = true;
                    }
                }
            }

            if (ImGui::MenuItem(ICON_FA_ROCKET " Build & Run Standalone"))
            {
                AppLaunchRuntimeEvent e;
                callback(e);
            }

            ImGui::EndMenu();
        }
    }

    void GUI::DrawMenuBar(const MenuBarState &state, const EventCallbackFn &callback)
    {
        if (ImGui::BeginMenuBar())
        {
            MenuFile(callback);
            MenuEdit(callback);
            MenuView(state, callback);
            MenuGame(state, callback);
            ImGui::EndMenuBar();
        }
    }

    void GUI::DrawToolbar(bool isPlaying, const EventCallbackFn &callback)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto &styleColors = ImGui::GetStyle().Colors;
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(styleColors[ImGuiCol_ButtonHovered].x, styleColors[ImGuiCol_ButtonHovered].y,
                                     styleColors[ImGuiCol_ButtonHovered].z, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(styleColors[ImGuiCol_ButtonActive].x, styleColors[ImGuiCol_ButtonActive].y,
                                     styleColors[ImGuiCol_ButtonActive].z, 0.5f));

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 32));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::Begin("##toolbar", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoNav);

        float toolbarSize = ImGui::GetWindowHeight() - 6.0f;
        float buttonsCount = isPlaying ? 2.0f : 2.0f; // Play/Pause or Stop/Pause
        float totalWidth = (toolbarSize * buttonsCount) + (ImGui::GetStyle().ItemSpacing.x * (buttonsCount - 1));

        ImGui::SetCursorPosX((ImGui::GetWindowWidth() * 0.5f) - (totalWidth * 0.5f));
        ImGui::SetCursorPosY(3.0f);

        // Play / Stop Button

        const char *icon = isPlaying ? ICON_FA_CIRCLE_STOP : ICON_FA_PLAY;
        ImVec4 iconColor = isPlaying ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        if (ImGui::Button(icon, ImVec2(toolbarSize, toolbarSize)))
        {
            if (isPlaying)
            {
                SceneStopEvent e;
                callback(e);
            }
            else
            {
                ScenePlayEvent e;
                callback(e);
            }
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(isPlaying ? "Stop (ESC)" : "Play");

        ImGui::SameLine();

        // Pause Button (Draft - needs engine support for Pause state)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));
            if (ImGui::Button(ICON_FA_PAUSE, ImVec2(toolbarSize, toolbarSize)))
            {
                // TODO: Dispatch Pause Event
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Pause Simulation");
        }

        // Standalone Launch (Only if not playing)
        if (!isPlaying)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - toolbarSize - 10.0f);
            if (ImGui::Button(ICON_FA_ROCKET, ImVec2(toolbarSize, toolbarSize)))
            {
                AppLaunchRuntimeEvent e;
                callback(e);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Launch Standalone Runtime");
        }

        if (isPlaying)
        {
            ImGui::SameLine();
            float textWidth = ImGui::CalcTextSize("Press ESC to Exit").x;
            float padding = 20.0f;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - textWidth - padding);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (toolbarSize * 0.2f));
            ImGui::Text("Press ESC to Exit");
            ImGui::PopStyleColor();
        }
        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

    void GUI::DrawProjectSelector(bool active, Texture2D icon, std::function<void()> onNew,
                                  std::function<void()> onOpen, std::function<void()> onExit)
    {
        if (active)
            return;

        ImGui::OpenPopup("Project Selector");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 300));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        if (ImGui::BeginPopupModal("Project Selector", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoTitleBar))
        {
            // Centralized Icon
            if (icon.id > 0)
            {
                float iconSize = 80.0f;
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - iconSize) * 0.5f);
                rlImGuiImageSize(&icon, (int)iconSize, (int)iconSize);
                ImGui::Spacing();
            }

            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Chained Decos Editor").x) * 0.5f);
            ImGui::Text("Chained Decos Editor");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Open Project", ImVec2(360, 45)))
            {
                if (onOpen)
                    onOpen();
            }
            ImGui::Spacing();
            if (ImGui::Button("New Project", ImVec2(360, 45)))
            {
                if (onNew)
                    onNew();
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Exit", ImVec2(360, 40)))
            {
                if (onExit)
                    onExit();
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

} // namespace CHEngine::EditorUI
