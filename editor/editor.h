#ifndef CH_EDITOR_H
#define CH_EDITOR_H

#include "engine/core/application.h"

namespace CHEngine
{
class Editor : public Application
{
public:
    Editor(const Application::Config &config);
    virtual ~Editor() = default;

    virtual void PostInitialize() override;
};
} // namespace CHEngine

#endif // CH_EDITOR_H
