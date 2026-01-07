#include "editor.h"
#include <engine/core/entry_point.h>


namespace CH
{
Application *CreateApplication()
{
    Application::Config config;
    config.Title = "Chained Editor";
    config.Width = 1600;
    config.Height = 900;

    return new Editor(config);
}
} // namespace CH
