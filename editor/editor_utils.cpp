#include "editor_utils.h"
#include "editor/editor_settings.h"
#include "engine/core/application.h"
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
    nfdfilteritem_t filterItem[1] = {{"Chained Project", "chproj"}};
    auto result = NFD::SaveDialog(outPath, filterItem, 1, nullptr, "Untitled.chproj");
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
}

void ProjectUtils::OpenProject()
{
    NFD::UniquePath outPath;
    nfdfilteritem_t filterItem[1] = {{"Chained Project", "chproj"}};
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
    }
}

void ProjectUtils::SaveProject()
{
    if (auto project = Project::GetActive())
    {
        ProjectSerializer serializer(project);
        std::filesystem::path path =
            project->GetProjectDirectory() / (project->GetConfig().Name + ".chproj");
        serializer.Serialize(path);
    }
}

// --- SceneUtils ---
void SceneUtils::NewScene(SceneType type)
{
    auto scene = std::make_shared<Scene>();
    scene->SetType(type);
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

    // Base
    auto &ui = entity.AddComponent<WidgetComponent>();
    ui.Transform.OffsetMin = {-100.0f, -40.0f};
    ui.Transform.OffsetMax = {100.0f, 40.0f};

    // Visual: Background
    auto &img = entity.AddComponent<ImageWidget>();
    img.BackgroundColor = {60, 60, 60, 255};
    img.Rounding = 4.0f;

    // Visual: Text
    auto &txt = entity.AddComponent<TextWidget>();
    txt.Text = label;
    txt.FontSize = 18.0f;
    txt.Color = {255, 255, 255, 255};

    // Logic: Button
    auto &btn = entity.AddComponent<ButtonWidget>();
    btn.Interactable = true;

    return entity;
}

Entity WidgetFactory::CreateText(Scene *scene, const std::string &text)
{
    auto entity = scene->CreateEntity(text);
    entity.AddComponent<WidgetComponent>();
    auto &txt = entity.AddComponent<TextWidget>();
    txt.Text = text;
    return entity;
}

Entity WidgetFactory::CreateImage(Scene *scene, const std::string &name)
{
    auto entity = scene->CreateEntity(name);
    entity.AddComponent<WidgetComponent>();
    auto &img = entity.AddComponent<ImageWidget>();
    img.BackgroundColor = {255, 255, 255, 255};
    return entity;
}

Entity WidgetFactory::CreateSlider(Scene *scene, const std::string &label)
{
    // 1. Root Slider
    auto sliderEntity = scene->CreateEntity(label);
    sliderEntity.AddComponent<WidgetComponent>();
    auto &slider = sliderEntity.AddComponent<SliderWidget>();
    slider.Min = 0.0f;
    slider.Max = 1.0f;

    // 2. Label Child
    auto textEntity = scene->CreateEntity("Label");
    auto &txtUI = textEntity.AddComponent<WidgetComponent>();
    txtUI.Transform.AnchorMin = {0.0f, 0.5f};
    txtUI.Transform.AnchorMax = {0.0f, 0.5f};
    txtUI.Transform.OffsetMin = {-100.00f, -10.0f};
    txtUI.Transform.OffsetMax = {-10.0f, 10.0f};
    txtUI.Transform.Pivot = {1.0f, 0.5f};
    auto &txt = textEntity.AddComponent<TextWidget>();
    txt.Text = label;
    SceneUtils::SetParent(textEntity, sliderEntity);

    return sliderEntity;
}

Entity WidgetFactory::CreateCheckbox(Scene *scene, const std::string &label)
{
    // 1. Root Checkbox
    auto checkboxEntity = scene->CreateEntity(label);
    checkboxEntity.AddComponent<WidgetComponent>();
    checkboxEntity.AddComponent<CheckboxWidget>();

    // 2. Label Child
    auto textEntity = scene->CreateEntity("Label");
    auto &txtUI = textEntity.AddComponent<WidgetComponent>();
    txtUI.Transform.AnchorMin = {1.0f, 0.5f}; // Right of checkbox
    txtUI.Transform.AnchorMax = {1.0f, 0.5f};
    txtUI.Transform.OffsetMin = {10.0f, -10.0f};
    txtUI.Transform.OffsetMax = {100.0f, 10.0f};
    txtUI.Transform.Pivot = {0.0f, 0.5f};
    auto &txt = textEntity.AddComponent<TextWidget>();
    txt.Text = label;
    SceneUtils::SetParent(textEntity, checkboxEntity);

    return checkboxEntity;
}
} // namespace CHEngine
