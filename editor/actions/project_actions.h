#ifndef CH_PROJECT_ACTIONS_H
#define CH_PROJECT_ACTIONS_H

#include "engine/scene/project.h"
#include <filesystem>
#include <string>

namespace CHEngine
{
class ProjectActions
{
public:
    static void New();
    static void New(const std::string& name, const std::string& path);
    static void Open();
    static void Open(const std::filesystem::path& path);
    static void Save();
    static void LaunchStandalone();
};
} // namespace CHEngine

#endif // CH_PROJECT_ACTIONS_H
