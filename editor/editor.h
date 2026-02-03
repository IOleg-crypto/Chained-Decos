#ifndef CH_EDITOR_H
#define CH_EDITOR_H

#include "engine/core/application.h"
#include <string>


namespace CHEngine
{
class Editor : public Application
{
public:
    Editor(const Application::Config &config);
    virtual ~Editor() = default;

    virtual void PostInitialize() override;
    void Shutdown();

    static Editor &Get()
    {
        return *(Editor *)&Application::Get();
    }

    struct EditorConfig
    {
        std::string LastProjectPath = "";
        std::string LastScenePath = "";
        bool LoadLastProjectOnStartup = false;
    };

    const EditorConfig &GetEditorConfig() const
    {
        return m_EditorConfig;
    }
    
    void SetLastProjectPath(const std::string &path)
    {
        m_EditorConfig.LastProjectPath = path;
    }
    
    void SetLastScenePath(const std::string &path)
    {
        m_EditorConfig.LastScenePath = path;
    }

    void LoadConfig();
    void SaveConfig();

private:
    EditorConfig m_EditorConfig;
};
} // namespace CHEngine

#endif // CH_EDITOR_H
