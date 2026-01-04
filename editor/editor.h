#ifndef CH_EDITOR_H
#define CH_EDITOR_H

#include "engine/core/application.h"

namespace CH
{
class Editor : public Application
{
public:
    Editor(const Application::Config &config);
    virtual ~Editor() = default;
};
} // namespace CH

#endif // CH_EDITOR_H
