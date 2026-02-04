#ifndef CH_RUNTIME_APPLICATION_H
#define CH_RUNTIME_APPLICATION_H

#include "engine/core/application.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class RuntimeApplication : public Application
{
public:
    RuntimeApplication(const ApplicationSpecification &specification, const std::string &projectPath = "");
    ~RuntimeApplication() override = default;

private:
    std::string m_ProjectPath;
    class Layer* m_RuntimeLayer = nullptr;
};
} // namespace CHEngine

#endif // CH_RUNTIME_APPLICATION_H
