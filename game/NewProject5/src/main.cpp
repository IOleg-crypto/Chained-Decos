
#include "engine/app/entry_point.h"
#include "engine/core/platform.h"
#include "engine/runtime/runtime_layer.h"

namespace Chained
{
	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "$PROJECT_NAME";
		spec.CommandLineArgs = args;
		spec.Headless = false;
		spec.EnableScripting = true;
		spec.EngineRoot = Platform::GetExecutableDirectory();
		spec.WorkingDirectory = Platform::GetExecutableDirectory().string();

		auto* app = new Application(spec);
		app->PushLayer(std::make_unique<RuntimeLayer>(spec.WorkingDirectory));
		return app;
	}
} // namespace Chained
