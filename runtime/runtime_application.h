#ifndef CH_RUNTIME_APPLICATION_H
#define CH_RUNTIME_APPLICATION_H

#include "engine/core/application.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class RuntimeApplication : public Application
{
public:
    RuntimeApplication(const Application::Config &config, const std::string &projectPath = "");
    virtual ~RuntimeApplication() = default;

private:
    Ref<Scene> m_ActiveScene;
};
} // namespace CHEngine

#endif // CH_RUNTIME_APPLICATION_H
