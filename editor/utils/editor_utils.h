
#ifndef CH_EDITOR_UTILS_H
#define CH_EDITOR_UTILS_H

#include <filesystem>
#include <memory>


namespace Chained
{
class Project;
class Scene;

void LaunchStandalone(std::shared_ptr<Project> project, std::shared_ptr<Scene> editorScene);

} // namespace Chained

#endif // CH_EDITOR_UTILS_H
