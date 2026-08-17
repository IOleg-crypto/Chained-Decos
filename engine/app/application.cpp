#include "engine/app/application.h"
#include "engine/core/profiler.h"
#include "engine/core/platform.h"
#include "engine/imgui/imgui_layer.h"
#include "engine/core/events/window_events.h"
#include "engine/core/service_locator.h"
#include "engine/scene/component_registry.h"
#include "engine/platform/dialogs/dialogs.h"
#include "engine/project/project.h"
#include "engine/common/thread_pool.h"
#include "engine/assets/asset_manager.h"
#include "engine/audio/audio.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/graphics/ui/ui_font_registry.h"
#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/physics/physics.h"
#include "engine/core/input.h"
#include "engine/scripting/scriptengine.h"
#include "engine/networking/network_service.h"

namespace Chained
{
	Application* Application::s_Instance = nullptr;

	std::filesystem::path Application::GetExecutableDirectory()
	{
		return Platform::GetExecutableDirectory();
	}

	Application::Application(const ApplicationSpecification& spec)
		: m_Specification(spec)
	{
		CH_ASSERT(!s_Instance);
		s_Instance = this;

		Log::Init();
		ComponentRegistry::RegisterEngineComponents();

		if (!m_Specification.WorkingDirectory.empty())
		{
			std::filesystem::current_path(m_Specification.WorkingDirectory);
		}

		// Window creation (not a Service — owns the OpenGL context)
		if (!m_Specification.Headless)
		{
			m_Window = Window::Create(m_Specification.Window);
			m_Window->SetEventCallback(CH_BIND_EVENT_FN(Application::OnEvent));
		}

		// Service registration - explicit order
		unsigned int threads = std::thread::hardware_concurrency();
		if (threads == 0)
		{
			threads = 1;
		}
		unsigned int workerCount = (threads > 1) ? (threads - 1) : 1;

		// Determine an optional explicit resources-directory override.
		// When ApplicationSpecification::ResourcesDir is set (e.g. via CMake for a
		// packaged/installed build), asset paths resolve there instead of the
		// default EngineRoot/ProjectDirectory search. When empty, the original
		// resolution is preserved: we must NOT call SetAssetDirectory, because asset
		// references already include the "resources/" prefix and would otherwise
		// double up (e.g. ".../resources/resources/...").
		std::filesystem::path resourcesDir;
		if (!m_Specification.ResourcesDir.empty() && std::filesystem::exists(m_Specification.ResourcesDir))
		{
			resourcesDir = m_Specification.ResourcesDir;
		}

		// 1. Input — must be first (window callbacks fire during creation)
		ServiceLocator::Provide<Core::Input>([] { return std::make_unique<Core::Input>(); });

		// 2. ThreadPool
		ServiceLocator::Provide<ThreadPool>([=] { return std::make_unique<ThreadPool>(workerCount); });

		// 3. AssetManager — override asset directory only when explicitly provided
		ServiceLocator::Provide<AssetManager>([&, resourcesDir] {
			auto am = std::make_unique<AssetManager>();
			am->SetEngineRoot(m_Specification.EngineRoot);
			if (!resourcesDir.empty())
			{
				am->SetAssetDirectory(resourcesDir);
			}
			return am;
		});

		// 4. Renderer (only when not headless)
		if (!m_Specification.Headless)
		{
			ServiceLocator::Provide<Renderer>([] { return std::make_unique<Renderer>(); });
			ServiceLocator::Provide<UIFontRegistry>([] { return std::make_unique<UIFontRegistry>(); });
			ServiceLocator::Provide<WidgetRenderer>([] { return std::make_unique<WidgetRenderer>(); });
			ServiceLocator::Provide<DebugRenderer>([] { return std::make_unique<DebugRenderer>(); });
		}

		// 5. Audio, Physics, ScriptEngine, Network
		ServiceLocator::Provide<Audio>([] { return std::make_unique<Audio>(); });
		ServiceLocator::Provide<Physics>([] { return std::make_unique<Physics>(); });
		ServiceLocator::Provide<ScriptEngine>(
			[=] { return std::make_unique<ScriptEngine>(m_Specification.EnableScripting); });
		ServiceLocator::Provide<Network>([] { return std::make_unique<Network>(); });

		// 6. Freeze the locator, then initialize all modules
		ServiceLocator::Lock();
		ServiceLocator::InitializeModule();

		if (m_Window)
		{
			if (auto* renderer = ServiceLocator::TryGet<Renderer>())
			{
				renderer->SetViewportSize(m_Window->GetWidth(), m_Window->GetHeight());
			}
		}

		m_LayerStack = std::make_unique<LayerStack>();
		m_Timer.LastFrameTime = Platform::GetTime();
		m_Running = true;

		if (!m_Specification.Headless)
		{
			auto imguiLayer = std::make_unique<ImGuiLayer>();
			m_ImGuiLayer = imguiLayer.get();
			PushOverlay(std::move(imguiLayer));
		}
	}

	Application::~Application()
	{

		m_LayerStack.reset();

		// WARNING: OpenGL resources held by the Project (like Environment maps)
		// MUST be released before the OpenGL context is destroyed (via m_Window.reset()).
		// Setting Active Project to nullptr invokes destructors cleanly.
		Project::SetActive(nullptr);

		ServiceLocator::Shutdown();
		m_Window.reset();
		// Log::Shutdown() MUST come after m_Window.reset(): ~GlfwWindow::Shutdown() emits
		// CH_CORE_INFO("Glfw Window Closed"). If the core logger were reset first, that log
		// call dereferences a null spdlog::logger and segfaults (access violation reading the
		// atomic log level). Logging outlives every subsystem that can log during teardown.
		Log::Shutdown();
		s_Instance = nullptr;
	}

	void Application::Run()
	{
		while (m_Running && (!m_Window || !m_Window->ShouldClose()))
		{
			float time = Platform::GetTime();
			float rawDelta = time - m_Timer.LastFrameTime;
			m_Timer.LastFrameTime = time;
			float clampedDelta = (rawDelta > 0.0f) ? std::min(rawDelta, 0.1f) : 0.0f;
			m_Timer.DeltaTime = Timestep(clampedDelta);

			Instrumentor::Get().BeginFrame();

			if (m_Window)
			{
				m_Window->BeginFrame();
			}

			if (m_Window && m_Window->GetWidth() > 0 && m_Window->GetHeight() > 0)
			{
				if (auto* audio = ServiceLocator::TryGet<Audio>())
				{
					audio->Update(m_Timer.DeltaTime);
				}
				if (auto* am = ServiceLocator::TryGet<AssetManager>())
				{
					am->Update(m_Timer.DeltaTime);
				}
				m_Timer.Accumulator += (float)m_Timer.DeltaTime;
				m_Timer.Accumulator = std::min(m_Timer.Accumulator, 0.1f);
				while (m_Timer.Accumulator >= m_Timer.FixedStepCount)
				{
					for (auto& layer : *m_LayerStack)
					{
						layer->OnFixedUpdate(Timestep(m_Timer.FixedStepCount));
					}

					m_Timer.Accumulator -= m_Timer.FixedStepCount;
				}

				for (auto& layer : *m_LayerStack)
				{
					layer->OnUpdate(m_Timer.DeltaTime);
				}

				Core::Input::Update(m_Timer.DeltaTime);

				for (auto& layer : *m_LayerStack)
				{
					layer->OnRender(m_Timer.DeltaTime);
				}

				if (m_ImGuiLayer)
				{
					m_ImGuiLayer->Begin();
					for (auto& layer : *m_LayerStack)
					{
						layer->OnImGuiRender();
					}
					m_ImGuiLayer->End();
				}

				m_Window->EndFrame();
			}
		}
	}

	void Application::OnEvent(Event& e)
	{
		if (e.GetEventType() == EventType::WindowResize)
		{
			auto& re = static_cast<WindowResizeEvent&>(e);
			if (auto* renderer = ServiceLocator::TryGet<Renderer>())
			{
				renderer->SetViewportSize(re.GetWidth(), re.GetHeight());
			}
		}

		for (auto it = m_LayerStack->rbegin(); it != m_LayerStack->rend(); ++it)
		{
			if (e.Handled)
			{
				break;
			}
			(*it)->OnEvent(e);
		}
	}

	void Application::PushLayer(std::unique_ptr<Layer> layer) const
	{
		Layer* raw = layer.get();
		m_LayerStack->PushLayer(std::move(layer));
		raw->OnAttach();
	}

	void Application::PushOverlay(std::unique_ptr<Layer> overlay) const
	{
		Layer* raw = overlay.get();
		m_LayerStack->PushOverlay(std::move(overlay));
		raw->OnAttach();
	}

} // namespace Chained