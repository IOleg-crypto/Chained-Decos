#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>


using json = nlohmann::json;

struct EditorSettings
{
    std::vector<std::string> recentProjects;
    static constexpr int MAX_RECENT = 10;

    void AddRecentProject(const std::string &path)
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

    json ToJson() const
    {
        return json{{"recentProjects", recentProjects}};
    }

    static EditorSettings FromJson(const json &j)
    {
        EditorSettings settings;
        if (j.contains("recentProjects") && j["recentProjects"].is_array())
        {
            settings.recentProjects = j["recentProjects"].get<std::vector<std::string>>();
        }
        return settings;
    }

    bool Save(const std::string &filepath)
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

    static EditorSettings Load(const std::string &filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (file.is_open())
            {
                json j;
                file >> j;
                return FromJson(j);
            }
        }
        catch (...)
        {
        }
        return EditorSettings();
    }
};

#endif // EDITOR_SETTINGS_H
