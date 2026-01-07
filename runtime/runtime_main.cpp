#include "runtime_application.h"
#include <engine/core/entry_point.h>


namespace CH
{
Application *CreateApplication()
{
    Application::Config config;
    config.Title = "Chained Runtime";
    config.Width = 1280;
    config.Height = 720;

    return new RuntimeApplication(config);
}
} // namespace CH
