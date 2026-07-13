#include "engine/platform/dialogs/file_dialogs.h"
#include "gui.h"
#include "editor/project/project_exporter.h"
#include "engine/core/service_locator.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "editor/layer.h"
#include "editor/panels/panel.h"
#include "editor/panels/viewport_panel.h"
#include "events.h"
#include "engine/app/application.h"
#include "engine/project/project.h"
#include "engine/scene/components.h"
#include "scripting/scriptengine.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "engine/app/application.h"
#include "engine/core/platform.h"
#include "engine/scene/component_registry.h"
#include "imgui_internal.h"
#include "scripting/scriptengine.h"
#include <filesystem>
#include <string>
#include <mutex>
#include "engine/common/thread_pool.h"

namespace Chained
{
// --- Internal Helpers ---

// State for the Export popup (static, ephemeral)
static struct ExportState
{
    bool        Open       = false;
    bool        Success    = false;
    std::string Message;
    std::string OutDir;
    std::mutex  Mutex;
    bool        IsExporting = false;
} s_ExportState;

static void DrawPropertyLabel(const char* label)
{
    const char* displayLabel = label ? label : "Unknown";
    if (ImGui::GetCurrentTable() != nullptr)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", displayLabel);
        ImGui::TableSetColumnIndex(1);
    }
    else
    {
        ImGui::Text("%s", displayLabel);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.4f);
    }
}

// --- Menu System Implementation ---

void EditorGUI::DrawMenuBar(EditorLayer& editorLayer, EditorPanels& panels)
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
            auto newScene = Scene::CreateDefault();
            EditorLayer::Get().GetSceneManager().SetScene(newScene);
        }
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project", "Ctrl+O"))
        {
            std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
            auto result = Chained::FileDialogs::OpenFile(filters);
            if (result)
            {
                EditorLayer::Get().GetSceneManager().OpenScene(*result);
            }
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project"))
        {
            EditorLayer::Get().GetSceneManager().SaveScene();
        }
        if (ImGui::MenuItem(ICON_FA_XMARK " Close Project"))
        {
            Project::SetActive(nullptr);
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_FILE_CODE " New Scene", "Ctrl+N"))
        {
            EditorLayer::Get().GetSceneManager().NewScene();
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene", "Ctrl+S"))
        {
            EditorLayer::Get().GetSceneManager().SaveScene();
        }
        if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save Scene As...", "Ctrl+Shift+S"))
        {
            EditorLayer::Get().GetSceneManager().SaveSceneAs();
        }
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Scene", "Ctrl+L"))
        {
            EditorLayer::Get().GetSceneManager().OpenScene();
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
        panels.ForEach([](const std::shared_ptr<Panel>& panel) {
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
        bool isExporting = false;
        {
            std::lock_guard<std::mutex> lock(s_ExportState.Mutex);
            isExporting = s_ExportState.IsExporting;
        }
        if (ImGui::MenuItem(isExporting ? ICON_FA_FILE_EXPORT " Exporting..." : ICON_FA_FILE_EXPORT " Export Project..."))
        {
            if (isExporting)
            {
                // Already exporting
            }
            else
            {
                auto outDir = FileDialogs::PickFolder();
                if (outDir)
                {
                    {
                        std::lock_guard<std::mutex> lock(s_ExportState.Mutex);
                        s_ExportState.IsExporting = true;
                    }
                    std::string outDirPath = outDir->string();
                    ServiceLocator::Get<ThreadPool>()->QueueTask([outDirPath]() {
                        auto result = ProjectExporter::ExportTo(outDirPath);
                        std::lock_guard<std::mutex> lock(s_ExportState.Mutex);
                        s_ExportState.Success = result.Success;
                        s_ExportState.Message = result.Success
                            ? "Export complete!"
                            : ("Export failed: " + result.Error);
                        s_ExportState.OutDir  = result.OutDir.string();
                        s_ExportState.Open    = true;
                        s_ExportState.IsExporting = false;
                    });
                }
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reload Shaders"))
        {
            ServiceLocator::Get<Renderer>()->GetShaderLibrary().ReloadAll();
        }
        if (ImGui::MenuItem(ICON_FA_FILE_CODE " Reload Scripts", "Ctrl+R"))
        {
            auto project = Project::GetActive();
            if (project)
            {
                auto assemblyPath = ScriptEngine::ResolveAssemblyPath(
                    project->GetConfig().Scripting, project->GetConfig().ProjectDirectory);
                ServiceLocator::Get<ScriptEngine>()->RequestAssemblyReload(assemblyPath.string(), "EditorGUI");
            }
        }
        ImGui::EndMenu();
    }

    // ── Export result popup ───────────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> lock(s_ExportState.Mutex);
        if (s_ExportState.Open)
        {
            ImGui::OpenPopup("Export Result");
            s_ExportState.Open = false;
        }
        if (ImGui::BeginPopupModal("Export Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (s_ExportState.Success)
            {
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), ICON_FA_CIRCLE_INFO " Success");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), ICON_FA_CIRCLE_EXCLAMATION " Failed");
            }
            ImGui::Spacing();
            ImGui::TextWrapped("%s", s_ExportState.Message.c_str());
            if (!s_ExportState.OutDir.empty())
            {
                ImGui::Spacing();
                ImGui::Text("Output: ");
                ImGui::SameLine();
                ImGui::TextDisabled("%s", s_ExportState.OutDir.c_str());
            }
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120.f, 0.f)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::EndMenuBar();
}

void EditorGUI::BeginPropertyGrid()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6)); // A bit more vertical breathing room
    ImGui::BeginTable("PropertyGrid", 2, 
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame);
    
    // Set left column to be roughly 40% of the width or 120px
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
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

template <typename F>
bool EditorGUI::PropertyWidget(const char* label, F&& widgetFn)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);
    bool changed = widgetFn();
    ImGui::PopID();
    return changed;
}

bool EditorGUI::Property(const char* label, bool& value)
{
    return PropertyWidget(label, [&]() { return ImGui::Checkbox("##prop", &value); });
}

bool EditorGUI::Property(const char* label, float& value, float speed, float min, float max)
{
    return PropertyWidget(label, [&]() { return ImGui::DragFloat("##prop", &value, speed, min, max); });
}

bool EditorGUI::Property(const char* label, int& value, int min, int max)
{
    return PropertyWidget(label, [&]() { return ImGui::DragInt("##prop", &value, 1.0f, min, max); });
}

bool EditorGUI::Property(const char* label, uint64_t& value)
{
    return PropertyWidget(label, [&]() { return ImGui::InputScalar("##prop", ImGuiDataType_U64, &value); });
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
    return PropertyWidget(label, [&]() {
        float c[4] = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
        bool changed = ImGui::ColorEdit4("##prop", c);
        if (changed)
        {
            value = {(unsigned char)(c[0] * 255), (unsigned char)(c[1] * 255), (unsigned char)(c[2] * 255),
                     (unsigned char)(c[3] * 255)};
        }
        return changed;
    });
}

bool EditorGUI::Property(const char* label, glm::vec2& value)
{
    return DrawVec2(label, value, 0.0f);
}
bool EditorGUI::Property(const char* label, glm::vec3& value)
{
    return DrawVec3(label, value, 0.0f);
}
bool EditorGUI::Property(const char* label, glm::vec4& value)
{
    return DrawVec4(label, value, 0.0f);
}

bool EditorGUI::PropertyColor(const char* label, glm::vec4& value, bool hdr)
{
    return PropertyWidget(label, [&]() {
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar;
        if (hdr)
        {
            flags |= ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float;
        }
        return ImGui::ColorEdit4("##prop", &value.x, flags);
    });
}

bool EditorGUI::Property(const char* label, int& value, const char** items, int itemCount)
{
    return PropertyWidget(label, [&]() { return ImGui::Combo("##prop", &value, items, itemCount); });
}

bool EditorGUI::FilePropertyImpl(const char* label, std::string& value,
    const char* filter, std::function<void()> thumbnailFn)
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);

    float width = ImGui::GetContentRegionAvail().x;
    float buttonSize = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;

    float thumbnailSize = 0.0f;
    if (thumbnailFn)
    {
        thumbnailSize = buttonSize * 1.5f;
        thumbnailFn();
        ImGui::SameLine();
    }

    ImGui::PushItemWidth(width - buttonSize - thumbnailSize - (thumbnailFn ? 10.0f : 5.0f));

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
        auto result = Chained::FileDialogs::OpenFile(filters);
        if (result)
        {
            value = Project::GetRelativePath(*result);
            changed = true;
        }
    }

    ImGui::PopID();
    return changed;
}

bool EditorGUI::FileProperty(const char* label, std::string& value, const char* filter)
{
    return FilePropertyImpl(label, value, filter, nullptr);
}

bool EditorGUI::FileProperty(const char* label, std::string& path, uint32_t textureId, const char* filter)
{
    return FilePropertyImpl(label, path, filter, [textureId]() {
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
    });
}

bool EditorGUI::ActionButton(const char* icon, const char* label)
{
    std::string text;
    if (icon && icon[0] != '\0')
    {
        text = std::string(icon) + " " + (label ? label : "");
    }
    else
    {
        text = (label ? label : "");
    }
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

template <int N>
bool EditorGUI::DrawVecImpl(const char* label, float* values, float resetValue,
    const ImVec4* colors, const char* componentLabels[N])
{
    DrawPropertyLabel(label);
    ImGui::PushID(label);

    bool changed = false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{4, 0});
    float width = ImGui::GetContentRegionAvail().x;
    float spacing = 4.0f * (N - 1);
    float itemWidth = (width - spacing) / N;

    ImGui::BeginGroup();

    for (int i = 0; i < N; ++i)
    {
        if (i > 0) ImGui::SameLine();
        ImGui::SetNextItemWidth(itemWidth);
        DrawPropertyControl(componentLabels[i], values[i], colors[i],
            componentLabels[i], resetValue, itemWidth, changed);
    }

    ImGui::EndGroup();

    ImGui::PopStyleVar();
    ImGui::PopID();
    return changed;
}

bool EditorGUI::DrawVec2(const char* label, glm::vec2& values, float resetValue)
{
    float arr[2] = {values.x, values.y};
    ImVec4 colors[2] = {{0.8f, 0.1f, 0.15f, 1.0f}, {0.2f, 0.7f, 0.2f, 1.0f}};
    const char* labels[2] = {"X", "Y"};
    bool changed = DrawVecImpl<2>(label, arr, resetValue, colors, labels);
    if (changed) { values.x = arr[0]; values.y = arr[1]; }
    return changed;
}

bool EditorGUI::DrawVec3(const char* label, glm::vec3& values, float resetValue)
{
    float arr[3] = {values.x, values.y, values.z};
    ImVec4 colors[3] = {{0.8f, 0.1f, 0.15f, 1.0f}, {0.2f, 0.7f, 0.2f, 1.0f}, {0.1f, 0.25f, 0.8f, 1.0f}};
    const char* labels[3] = {"X", "Y", "Z"};
    bool changed = DrawVecImpl<3>(label, arr, resetValue, colors, labels);
    if (changed) { values.x = arr[0]; values.y = arr[1]; values.z = arr[2]; }
    return changed;
}

bool EditorGUI::DrawVec4(const char* label, glm::vec4& values, float resetValue)
{
    float arr[4] = {values.x, values.y, values.z, values.w};
    ImVec4 colors[4] = {{0.8f, 0.1f, 0.15f, 1.0f}, {0.2f, 0.7f, 0.2f, 1.0f}, {0.1f, 0.25f, 0.8f, 1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}};
    const char* labels[4] = {"X", "Y", "Z", "W"};
    bool changed = DrawVecImpl<4>(label, arr, resetValue, colors, labels);
    if (changed) { values.x = arr[0]; values.y = arr[1]; values.z = arr[2]; values.w = arr[3]; }
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
} // namespace Chained