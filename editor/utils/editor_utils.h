
#ifndef CH_EDITOR_UTILS_H
#define CH_EDITOR_UTILS_H

#include <memory>
#include <filesystem>

namespace CHEngine
{
class Project;
class Scene;

void LaunchStandalone(std::shared_ptr<Project> project, std::shared_ptr<Scene> editorScene);

} // namespace CHEngine

#endif // CH_EDITOR_UTILS_H
