#ifndef CH_EDITOR_UTILS_H
#define CH_EDITOR_UTILS_H

#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include <filesystem>
#include <string>

namespace CHEngine
{
class ProjectUtils
{
public:
    static void NewProject();
    static void NewProject(const std::string &name, const std::string &path);

    static void OpenProject();
    static void OpenProject(const std::filesystem::path &path);

    static void SaveProject();
};

class SceneUtils
{
public:
    static void NewScene();
    static void OpenScene();
    static void OpenScene(const std::filesystem::path &path);
    static void SaveScene();
    static void SaveSceneAs();
    static void SetParent(Entity child, Entity parent);
};

class WidgetFactory
{
public:
    static Entity CreateButton(Scene *scene, const std::string &label = "Button");
    static Entity CreateLabel(Scene *scene, const std::string &text = "Text Label");
    static Entity CreatePanel(Scene *scene, const std::string &name = "Panel");
    static Entity CreateSlider(Scene *scene, const std::string &label = "Slider");
    static Entity CreateCheckbox(Scene *scene, const std::string &label = "Checkbox");
};
} // namespace CHEngine

#endif // CH_EDITOR_UTILS_H
