#ifndef CH_EDITOR_SETTINGS_H
#define CH_EDITOR_SETTINGS_H

#include "engine/core/base.h"
#include <filesystem>
#include <string>

namespace CHEngine
{
struct EditorSettingsData
{
    int WindowWidth = 1600;
    int WindowHeight = 900;
    bool Fullscreen = false;
    int TargetFPS = 144;
    bool VSync = true;
    std::string LastProjectPath = "";
    std::string LastScenePath = "";
};

class EditorSettings
{
public:
    static void Init();
    static void Shutdown();

    static const EditorSettingsData &Get()
    {
        return s_Data;
    }
    static void Save();
    static void Load();

    static void SetWindowSize(int width, int height)
    {
        s_Data.WindowWidth = width;
        s_Data.WindowHeight = height;
    }
    static void SetFullscreen(bool fullscreen)
    {
        s_Data.Fullscreen = fullscreen;
    }
    static void SetLastProjectPath(const std::string &path)
    {
        s_Data.LastProjectPath = path;
    }
    static void SetLastScenePath(const std::string &path)
    {
        s_Data.LastScenePath = path;
    }

private:
    static EditorSettingsData s_Data;
    static std::filesystem::path GetConfigPath();
};
} // namespace CHEngine

#endif // CH_EDITOR_SETTINGS_H
