#include "editor_settings.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

void EditorSettings::AddRecentProject(const std::string &path)
{
    if (path.empty())
        return;

    // Remove if exists to move to top
    recentProjects.erase(std::remove(recentProjects.begin(), recentProjects.end(), path),
                         recentProjects.end());

    // Insert at beginning
    recentProjects.insert(recentProjects.begin(), path);

    // Limit size
    if (recentProjects.size() > MAX_RECENT)
    {
        recentProjects.resize(MAX_RECENT);
    }
}

nlohmann::json EditorSettings::ToJson() const
{
    return nlohmann::json{{"recentProjects", recentProjects}};
}

EditorSettings EditorSettings::FromJson(const nlohmann::json &j)
{
    EditorSettings settings;
    if (j.contains("recentProjects") && j["recentProjects"].is_array())
    {
        settings.recentProjects = j["recentProjects"].get<std::vector<std::string>>();
    }
    return settings;
}

bool EditorSettings::Save(const std::string &filepath)
{
    try
    {
        std::ofstream file(filepath);
        if (file.is_open())
        {
            file << ToJson().dump(4);
            return true;
        }
    }
    catch (...)
    {
    }
    return false;
}

EditorSettings EditorSettings::Load(const std::string &filepath)
{
    try
    {
        std::ifstream file(filepath);
        if (file.is_open())
        {
            nlohmann::json j;
            file >> j;
            return FromJson(j);
        }
    }
    catch (...)
    {
    }
    return EditorSettings();
}
