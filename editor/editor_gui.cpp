#include "editor_gui.h"
#include "editor/editor_layer.h"
#include "editor/panels/panel.h"
#include "editor/panels/viewport_panel.h"
#include "editor_events.h"
#include "engine/core/application.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "imgui/IconsFontAwesome6.h"
#include "scripting/scriptengine.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "engine/core/dialogs.h"
#include "engine/scene/scene_picking.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstring>
#include <filesystem>
#include <string>

namespace CHEngine
{
// --- Internal Helpers ---

static void DrawPropertyLabel(const char* label)
{
    if (ImGui::GetCurrentTable() != nullptr)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::Text(label);
        ImGui::TableSetColumnIndex(1);
    }
    else
    {
        ImGui::Text(label);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.4f);
    }
}

// --- Menu System Implementation ---

void EditorGUI::DrawMenuBar(EditorPanels& panels)
{
    if (!ImGui::BeginMenuBar())
    {
        return;
    }

    // File Menu
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem(ICON_FA_FILE " New Project", "Ctrl+Shift+N"))
        {
            auto newScene = std::make_shared<Scene>();

            // Ensure every scene starts with a Main Camera
            Entity camera = newScene->CreateEntity("Main Camera");
            auto& cc = camera.AddComponent<CameraComponent>();
            cc.Primary = true;
            camera.GetComponent<TransformComponent>().Translation = {0, 5, 10};

            EditorLayer::Get().SetScene(newScene);
        }
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project", "Ctrl+O"))
        {
            std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
            auto result = Dialogs::OpenFile(filters);
            if (result)
            {
                EditorLayer::Get().OpenScene(*result);
            }
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project"))
        {
            EditorLayer::Get().SaveScene();
        }
        if (ImGui::MenuItem(ICON_FA_XMARK " Close Project"))
        {
            Project::SetActive(nullptr);
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_FILE_CODE " New Scene", "Ctrl+N"))
        {
            EditorLayer::Get().NewScene();
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene", "Ctrl+S"))
        {
            EditorLayer::Get().SaveScene();
        }
        if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save Scene As...", "Ctrl+Shift+S"))
        {
            EditorLayer::Get().SaveSceneAs();
        }
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Scene", "Ctrl+L"))
        {
            EditorLayer::Get().OpenScene();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_POWER_OFF " Exit"))
        {
            Application::Get().Close();
        }
        ImGui::EndMenu();
    }

    // View Menu
    if (ImGui::BeginMenu("View"))
    {
        panels.ForEach([](std::shared_ptr<Panel> panel) {
            if (panel->GetName() != "Viewport" && panel->GetName() != "Project Browser")
            {
                ImGui::MenuItem(panel->GetName().c_str(), nullptr, &panel->IsOpen());
            }
        });
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_EXPAND " Fullscreen", "F11"))
        {
            Application::Get().GetWindow().ToggleFullscreen();
        }
        if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reset Layout"))
        {
            AppResetLayoutEvent e;
            Application::Get().OnEvent(e);
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Layout"))
        {
            AppSaveLayoutEvent e;
            Application::Get().OnEvent(e);
        }
        ImGui::EndMenu();
    }

    // Project Menu
    if (ImGui::BeginMenu("Project"))
    {
        if (ImGui::MenuItem(ICON_FA_GEARS " Settings"))
        {
            if (auto p = panels.Get("Project Settings"))
            {
                p->IsOpen() = true;
            }
        }
        if (ImGui::MenuItem(ICON_FA_ROCKET " Build & Run"))
        {
            AppLaunchRuntimeEvent e;
            Application::Get().OnEvent(e);
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reload Shaders"))
        {
            Renderer::Get().GetShaderLibrary().ReloadAll();
        }
        if (ImGui::MenuItem(ICON_FA_FILE_CODE " Reload Scripts", "Ctrl+R"))
        {
            auto& scriptEngine = ScriptEngine::Get();
            if (scriptEngine.IsReloadInProgress())
            {
                CH_CORE_INFO("EditorGUI: Script reload request ignored (reload already in progress).");
            }
            else if (!scriptEngine.ReloadAssembly())
            {
                CH_CORE_WARN("EditorGUI: Script reload failed from menu action.");
            }
        }
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void EditorGUI::BeginPropertyGrid()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::BeginTable("PropertyGrid", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame);
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);
}

void EditorGUI::EndPropertyGrid()
{
    ImGui::EndTable();
    ImGui::PopStyleVar();
}

void EditorGUI::BeginProperty(const char* label)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    ImGui::PushItemWidth(-1);
}

void EditorGUI::EndProperty()
{
    ImGui::PopItemWidth();
    ImGui::PopID();
}

// --- Property Widgets Implementation (New Unified Style) ---

bool EditorGUI::Property(const char* label, bool& value)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    bool changed = ImGui::Checkbox("##prop", &value);
    ImGui::PopID();
    return changed;
}

bool EditorGUI::Property(const char* label, float& value, float speed, float min, float max)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    bool changed = ImGui::DragFloat("##prop", &value, speed, min, max);
    ImGui::PopID();
    return changed;
}

bool EditorGUI::Property(const char* label, int& value, int min, int max)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    bool changed = ImGui::DragInt("##prop", &value, 1.0f, min, max);
    ImGui::PopID();
    return changed;
}

bool EditorGUI::Property(const char* label, uint64_t& value)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    bool changed = ImGui::InputScalar("##prop", ImGuiDataType_U64, &value);
    ImGui::PopID();
    return changed;
}

bool EditorGUI::Property(const char* label, std::string& value, bool multiline)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    bool changed = false;
    if (multiline)
    {
        if (ImGui::InputTextMultiline("##prop", buffer, sizeof(buffer),
                                      ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 3)))
        {
            value = buffer;
            changed = true;
        }
    }
    else
    {
        if (ImGui::InputText("##prop", buffer, sizeof(buffer)))
        {
            value = buffer;
            changed = true;
        }
    }
    ImGui::PopID();
    return changed;
}

bool EditorGUI::Property(const char* label, Color& value)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    float c[4] = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
    bool changed = ImGui::ColorEdit4("##prop", c);
    if (changed)
    {
        value = {(unsigned char)(c[0] * 255), (unsigned char)(c[1] * 255), (unsigned char)(c[2] * 255),
                 (unsigned char)(c[3] * 255)};
    }
    ImGui::PopID();
    return changed;
}

bool EditorGUI::Property(const char* label, glm::vec2& value, float speed, float min, float max)
{
    return DrawVec2(label, value, 0.0f);
}
bool EditorGUI::Property(const char* label, glm::vec3& value, float speed, float min, float max)
{
    return DrawVec3(label, value, 0.0f);
}
bool EditorGUI::Property(const char* label, glm::vec4& value, float speed, float min, float max)
{
    return DrawVec4(label, value, 0.0f);
}

bool EditorGUI::Property(const char* label, int& value, const char** items, int itemCount)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    bool changed = ImGui::Combo("##prop", &value, items, itemCount);
    ImGui::PopID();
    return changed;
}

bool EditorGUI::FileProperty(const char* label, std::string& value, const char* filter)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    float width = ImGui::GetContentRegionAvail().x;
    float buttonSize = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    ImGui::PushItemWidth(width - buttonSize - 5.0f);
    std::string displayPath = Project::GetRelativePath(value);
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, displayPath.c_str(), sizeof(buffer) - 1);
    
    bool changed = false;
    if (ImGui::InputText("##prop", buffer, sizeof(buffer)))
    {
        value = Project::GetAbsolutePath(buffer).string();
        changed = true;
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* dropPath = (const char*)payload->Data;
            value = Project::GetRelativePath(dropPath);
            changed = true;
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_OPEN, {buttonSize, buttonSize}))
    {
        std::vector<FileDialogFilter> filters;
        if (filter != nullptr && filter[0] != '\0')
        {
            filters.push_back({"Files", filter});
        }
        auto result = Dialogs::OpenFile(filters);
        if (result)
        {
            value = Project::GetRelativePath(*result);
            changed = true;
        }
    }
    ImGui::PopID();
    return changed;
}

bool EditorGUI::FileProperty(const char* label, std::string& path, uint32_t textureId, const char* filter)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    float width = ImGui::GetContentRegionAvail().x;
    float buttonSize = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    float thumbnailSize = buttonSize * 1.5f;
    if (textureId > 0)
    {
        ImGui::Image((void*)(intptr_t)textureId, {thumbnailSize, thumbnailSize}, {0, 1}, {1, 0});
    }
    else
    {
        ImGui::Button("##empty", {thumbnailSize, thumbnailSize});
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("No texture loaded");
        }
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(width - buttonSize - thumbnailSize - 10.0f);
    
    std::string displayPath = Project::GetRelativePath(path);
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, displayPath.c_str(), sizeof(buffer) - 1);
    
    bool changed = false;
    if (ImGui::InputText("##prop", buffer, sizeof(buffer)))
    {
        path = Project::GetAbsolutePath(buffer).string();
        changed = true;
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* dropPath = (const char*)payload->Data;
            path = Project::GetRelativePath(dropPath);
            changed = true;
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_OPEN, {buttonSize, buttonSize}))
    {
        std::vector<FileDialogFilter> filters;
        if (filter != nullptr && filter[0] != '\0')
        {
            filters.push_back({"Files", filter});
        }
        auto result = Dialogs::OpenFile(filters);
        if (result)
        {
            path = Project::GetRelativePath(*result);
            changed = true;
        }
    }
    ImGui::PopID();
    return changed;
}

bool EditorGUI::ActionButton(const char* icon, const char* label)
{
    std::string text = std::string(icon) + " " + label;
    return ImGui::Button(text.c_str());
}

static void DrawPropertyControl(const char* id, float& val, ImVec4 color, const char* label, float resetValue,
                                float width, bool& changed)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushID(label);

    float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight, lineHeight};

    // Label with background color
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    // Use a button as a colored label
    if (ImGui::Button(label, buttonSize))
    {
        val = resetValue;
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Click to reset to %.2f", resetValue);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, 0); // No spacing between label and input

    ImGui::SetNextItemWidth(width - buttonSize.x);
    char buf[32];
    sprintf(buf, "##%s_%s", label, id);
    if (ImGui::DragFloat(buf, &val, 0.1f, 0.0f, 0.0f, "%.2f"))
    {
        changed = true;
    }

    ImGui::PopID();
}

bool EditorGUI::DrawVec3(const char* label, glm::vec3& values, float resetValue)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);

    bool changed = false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{4, 0});
    float width = ImGui::GetContentRegionAvail().x;
    float itemWidth = (width - 8.0f) / 3.0f; // 4px spacing * 2

    ImGui::BeginGroup();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("x", values.x, {0.8f, 0.1f, 0.15f, 1.0f}, "X", resetValue, itemWidth, changed);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("y", values.y, {0.2f, 0.7f, 0.2f, 1.0f}, "Y", resetValue, itemWidth, changed);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("z", values.z, {0.1f, 0.25f, 0.8f, 1.0f}, "Z", resetValue, itemWidth, changed);

    ImGui::EndGroup();

    ImGui::PopStyleVar();
    ImGui::PopID();
    return changed;
}

bool EditorGUI::DrawVec2(const char* label, glm::vec2& values, float resetValue)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);

    bool changed = false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{4, 0});
    float width = ImGui::GetContentRegionAvail().x;
    float itemWidth = (width - 4.0f) / 2.0f;

    ImGui::BeginGroup();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("x", values.x, {0.8f, 0.1f, 0.15f, 1.0f}, "X", resetValue, itemWidth, changed);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("y", values.y, {0.2f, 0.7f, 0.2f, 1.0f}, "Y", resetValue, itemWidth, changed);

    ImGui::EndGroup();

    ImGui::PopStyleVar();
    ImGui::PopID();
    return changed;
}

bool EditorGUI::DrawVec4(const char* label, glm::vec4& values, float resetValue)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);

    bool changed = false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{4, 0});
    float width = ImGui::GetContentRegionAvail().x;
    float itemWidth = (width - 12.0f) / 4.0f;

    ImGui::BeginGroup();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("x", values.x, {0.8f, 0.1f, 0.15f, 1.0f}, "X", resetValue, itemWidth, changed);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("y", values.y, {0.2f, 0.7f, 0.2f, 1.0f}, "Y", resetValue, itemWidth, changed);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("z", values.z, {0.1f, 0.25f, 0.8f, 1.0f}, "Z", resetValue, itemWidth, changed);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(itemWidth);
    DrawPropertyControl("w", values.w, {0.5f, 0.5f, 0.5f, 1.0f}, "W", resetValue, itemWidth, changed);

    ImGui::EndGroup();

    ImGui::PopStyleVar();
    ImGui::PopID();
    return changed;
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
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.10f, 0.12f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.25f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.08f, 0.10f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.35f, 0.60f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.35f, 0.50f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.40f, 0.60f, 1.00f);

    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.45f, 0.70f, 1.00f);

    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.35f, 0.50f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
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

Ray EditorGUI::GetMouseRay(const Camera3D& camera, const glm::vec2& mousePosition, const glm::vec2& viewportSize)
{
    return ScenePicker::CreateRayFromViewport(camera, mousePosition, viewportSize);
}

} // namespace CHEngine
