#include "editor_utils.h"
#include "editor/editor_settings.h"
#include "engine/core/application.h"
#include "engine/core/events.h"
#include "engine/core/log.h"
#include "engine/scene/components/widget_component.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
#include "nfd.hpp"
#include <filesystem>
#include <fstream>

namespace CHEngine
{
// --- ProjectUtils ---
void ProjectUtils::NewProject()
{
    NFD::UniquePath outPath;
    nfdfilteritem_t filterItem[1] = {{"Chained Project", "chproject"}};
    auto result = NFD::SaveDialog(outPath, filterItem, 1, nullptr, "Untitled.chproject");
    if (result == NFD_OKAY)
    {
        std::filesystem::path p = outPath.get();
        NewProject(p.stem().string(), p.parent_path().string());
    }
}

void ProjectUtils::NewProject(const std::string &name, const std::string &path)
{
    auto project = std::make_shared<Project>();
    project->SetName(name);
    project->SetProjectDirectory(path);
    Project::SetActive(project);
    ProjectUtils::SaveProject();

    // Dispatch event so panels can update
    std::filesystem::path projectFile = std::filesystem::path(path) / (name + ".chproject");
    ProjectOpenedEvent e(projectFile.string());
    Application::Get().OnEvent(e);
}

void ProjectUtils::OpenProject()
{
    NFD::UniquePath outPath;
    nfdfilteritem_t filterItem[1] = {{"Chained Project", "chproject"}};
    auto result = NFD::OpenDialog(outPath, filterItem, 1);
    if (result == NFD_OKAY)
    {
        OpenProject(outPath.get());
    }
}

void ProjectUtils::OpenProject(const std::filesystem::path &path)
{
    auto project = std::make_shared<Project>();
    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        Project::SetActive(project);
        ProjectOpenedEvent e(path.string());
        Application::Get().OnEvent(e);
    }
}

void ProjectUtils::SaveProject()
{
    if (auto project = Project::GetActive())
    {
        ProjectSerializer serializer(project);
        std::filesystem::path path =
            project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
        serializer.Serialize(path);
    }
}

// --- SceneUtils ---
void SceneUtils::NewScene()
{
    auto scene = std::make_shared<Scene>();
    Application::Get().SetActiveScene(scene);

    // Notify system (Panels need this to update context)
    SceneOpenedEvent e("");
    Application::OnEvent(e);
}

void SceneUtils::OpenScene()
{
    NFD::UniquePath outPath;
    nfdfilteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
    auto result = NFD::OpenDialog(outPath, filterItem, 1);
    if (result == NFD_OKAY)
    {
        OpenScene(outPath.get());
    }
}

void SceneUtils::OpenScene(const std::filesystem::path &path)
{
    Application::Get().LoadScene(path.string());
}

void SceneUtils::SaveScene()
{
    if (auto scene = Application::Get().GetActiveScene())
    {
        SaveSceneAs();
    }
}

void SceneUtils::SaveSceneAs()
{
    if (auto scene = Application::Get().GetActiveScene())
    {
        NFD::UniquePath outPath;
        nfdfilteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
        auto result = NFD::SaveDialog(outPath, filterItem, 1, nullptr, "Untitled.chscene");
        if (result == NFD_OKAY)
        {
            SceneSerializer serializer(scene.get());
            serializer.Serialize(outPath.get());
        }
    }
}

void SceneUtils::SetParent(Entity child, Entity parent)
{
    if (!child || !parent)
        return;

    auto &chc = child.HasComponent<HierarchyComponent>() ? child.GetComponent<HierarchyComponent>()
                                                         : child.AddComponent<HierarchyComponent>();
    chc.Parent = parent;

    auto &phc = parent.HasComponent<HierarchyComponent>()
                    ? parent.GetComponent<HierarchyComponent>()
                    : parent.AddComponent<HierarchyComponent>();
    phc.Children.push_back(child);
}

// --- WidgetFactory ---

Entity WidgetFactory::CreateButton(Scene *scene, const std::string &label)
{
    auto entity = scene->CreateEntity(label);

    auto &ui = entity.AddComponent<WidgetComponent>();
    ui.Transform.OffsetMin = {-80.0f, -25.0f};
    ui.Transform.OffsetMax = {80.0f, 25.0f};

    auto &btn = entity.AddComponent<ButtonWidget>();
    btn.Label = label;

    // Modern default style
    btn.Style.BackgroundColor = {60, 60, 65, 255};
    btn.Style.HoverColor = {80, 80, 85, 255};
    btn.Style.PressedColor = {40, 40, 45, 255};
    btn.Style.Rounding = 6.0f;

    btn.Text.FontSize = 20.0f;
    btn.Text.TextColor = WHITE;
    btn.Text.bShadow = true;

    return entity;
}

Entity WidgetFactory::CreateLabel(Scene *scene, const std::string &text)
{
    auto entity = scene->CreateEntity(text);
    entity.AddComponent<WidgetComponent>();
    auto &lbl = entity.AddComponent<LabelWidget>();
    lbl.Text = text;
    lbl.Style.FontSize = 22.0f;
    lbl.Style.TextColor = WHITE;
    return entity;
}

Entity WidgetFactory::CreatePanel(Scene *scene, const std::string &name)
{
    auto entity = scene->CreateEntity(name);
    auto &ui = entity.AddComponent<WidgetComponent>();
    ui.Transform.OffsetMin = {-200.0f, -150.0f};
    ui.Transform.OffsetMax = {200.0f, 150.0f};

    auto &pnl = entity.AddComponent<PanelWidget>();
    pnl.Style.BackgroundColor = {30, 30, 32, 180}; // Semi-transparent dark
    pnl.Style.Rounding = 12.0f;
    pnl.Style.BorderSize = 1.0f;
    pnl.Style.BorderColor = {100, 100, 105, 255};

    return entity;
}

Entity WidgetFactory::CreateSlider(Scene *scene, const std::string &label)
{
    auto entity = scene->CreateEntity(label);
    entity.AddComponent<WidgetComponent>();
    auto &slider = entity.AddComponent<SliderWidget>();
    slider.Min = 0.0f;
    slider.Max = 1.0f;
    slider.Value = 0.5f;
    return entity;
}

Entity WidgetFactory::CreateCheckbox(Scene *scene, const std::string &label)
{
    auto entity = scene->CreateEntity(label);
    entity.AddComponent<WidgetComponent>();
    entity.AddComponent<CheckboxWidget>();
    return entity;
}
} // namespace CHEngine
