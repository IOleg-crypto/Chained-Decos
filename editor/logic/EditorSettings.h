#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

struct EditorSettings
{
    std::vector<std::string> recentProjects;
    static constexpr int MAX_RECENT = 10;

    void AddRecentProject(const std::string &path);
    nlohmann::json ToJson() const;
    static EditorSettings FromJson(const nlohmann::json &j);
    bool Save(const std::string &filepath);
    static EditorSettings Load(const std::string &filepath);
};

#endif // EDITOR_SETTINGS_H
