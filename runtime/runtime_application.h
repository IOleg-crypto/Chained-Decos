#ifndef CH_RUNTIME_APPLICATION_H
#define CH_RUNTIME_APPLICATION_H

#include "engine/core/application.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class RuntimeApplication : public Application
{
public:
    using ScriptRegistrationCallback = std::function<void(Scene*)>;

    RuntimeApplication(const ApplicationSpecification &specification, 
                       const std::string &projectPath = "", 
                       ScriptRegistrationCallback scriptCallback = nullptr);
    ~RuntimeApplication() override = default;

    // Staged Life Cycle
    bool InitProject(const std::string& projectPath);
    bool LoadModule();
    void LoadScene(const std::string& scenePath);
    void LoadScene(int index);

private:
    std::string m_ProjectPath;
    std::filesystem::path m_EngineRoot;
    class Layer* m_RuntimeLayer = nullptr;
    ScriptRegistrationCallback m_ScriptCallback;
};
} // namespace CHEngine

#endif // CH_RUNTIME_APPLICATION_H
