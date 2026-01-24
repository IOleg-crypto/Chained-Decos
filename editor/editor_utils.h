#ifndef CH_EDITOR_UTILS_H
#define CH_EDITOR_UTILS_H

#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include <filesystem>
#include <string>

namespace CHEngine::EditorUtils
{
namespace ProjectUtils
{
void NewProject();
void NewProject(const std::string &name, const std::string &path);

void OpenProject();
void OpenProject(const std::filesystem::path &path);

void SaveProject();
} // namespace ProjectUtils

namespace SceneUtils
{
void NewScene();
void OpenScene();
void OpenScene(const std::filesystem::path &path);
void SaveScene();
void SaveSceneAs();
} // namespace SceneUtils
} // namespace CHEngine::EditorUtils

#endif // CH_EDITOR_UTILS_H
