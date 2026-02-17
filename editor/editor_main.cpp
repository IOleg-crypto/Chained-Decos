#include "engine/core/application.h"
#include "editor_layer.h"
#include "engine/core/entry_point.h"

namespace CHEngine
{
Application *CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.Name = "Chained Editor";
    spec.CommandLineArgs = args;

    auto app = new Application(spec);
    app->PushLayer(new EditorLayer());
    return app;
}
} // namespace CHEngine
