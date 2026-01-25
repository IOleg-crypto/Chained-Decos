#include "menu_bar.h"
#include "editor/editor_utils.h"
#include "engine/core/application.h"
#include "extras/IconsFontAwesome6.h"
#include "panels/panel.h"
#include "panels/viewport_panel.h"
#include <imgui.h>

namespace CHEngine
{
namespace UI
{

static void MenuFile(const EventCallbackFn &callback)
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem(ICON_FA_FILE " New Project", "Ctrl+Shift+N"))
            ProjectUtils::NewProject();
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project", "Ctrl+O"))
            ProjectUtils::OpenProject();
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project"))
            ProjectUtils::SaveProject();
        if (ImGui::MenuItem(ICON_FA_XMARK " Close Project"))
            Project::SetActive(nullptr);

        ImGui::Separator();

        if (ImGui::BeginMenu(ICON_FA_FILE_CODE " New Scene"))
        {
            if (ImGui::MenuItem("3D Scene"))
                SceneUtils::NewScene(SceneType::Scene3D);
            if (ImGui::MenuItem("UI Menu"))
                SceneUtils::NewScene(SceneType::SceneUI);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene", "Ctrl+S"))
            SceneUtils::SaveScene();
        if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save Scene As...", "Ctrl+Shift+S"))
            SceneUtils::SaveSceneAs();
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Scene", "Ctrl+L"))
            SceneUtils::OpenScene();

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
            // Layer will handle shortcuts, but we can call directly if we have instance
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

void DrawMenuBar(const MenuBarState &state, const EventCallbackFn &callback)
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

} // namespace UI
} // namespace CHEngine
