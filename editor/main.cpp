#include "engine/app/entry_point.h"
#include "engine/core/platform.h"
#include "layer.h"

namespace Chained
{
	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "ChainedEditor";
		spec.CommandLineArgs = args;
		spec.Headless = false;

		// Default editor window settings
		spec.Window.Width = 0;
		spec.Window.Height = 0;
		spec.Window.Fullscreen = false;
		spec.EnableScripting = true;

		// Set engine root to the executable directory so AssetManager can find
		// resources/shaders, resources/icons, resources/font etc.
		spec.EngineRoot = Platform::GetExecutableDirectory();
		spec.WorkingDirectory = Platform::GetExecutableDirectory().string();

		auto* app = new Application(spec);

		app->PushLayer(std::make_unique<EditorLayer>());

		return app;
	}
} // namespace Chained
