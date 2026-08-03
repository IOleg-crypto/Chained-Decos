// runtime_layer.cpp
// Chained Engine — Runtime layer for standalone game execution.
// Handles project loading, scene transitions, scripting lifecycle, and the main runtime loop.

#include "runtime_layer.h"
#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "engine/common/asset_path.h"
#include "engine/core/events/window_events.h"
#include "engine/core/platform.h"
#include "engine/core/service_locator.h"
#include "engine/core/window.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/imgui/imgui_layer.h"
#include "engine/physics/physics.h"
#include "engine/project/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_serializer.h"
#include "imgui.h"
#include "scripting/scene_scripting_manager.h"
#include "scripting/scriptengine.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <unordered_set>

namespace Chained
{

	RuntimeLayer::RuntimeLayer(const std::string& projectPath)
		: Layer("RuntimeLayer"),
		  m_ProjectPath(projectPath)
	{
		m_SceneRenderer = std::make_unique<SceneRenderer>();

		m_Renderer = ServiceLocator::TryGet<Renderer>();
		m_AssetManager = ServiceLocator::TryGet<AssetManager>();
	}

	RuntimeLayer::~RuntimeLayer() = default;

	void RuntimeLayer::OnAttach()
	{
		auto* imguiLayer = Application::Get().GetImGuiLayer();
		if (imguiLayer)
		{
			ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiLayer->GetContext()));
		}

		auto& io = ImGui::GetIO();

		if (io.Fonts->Fonts.Size == 0)
		{
			io.Fonts->AddFontDefault();
			CH_CORE_INFO("RuntimeSystem: Using built-in ImGui default font.");
		}

		InitProject(m_ProjectPath);

		if (auto* wr = ServiceLocator::TryGet<WidgetRenderer>())
		{
			if (ImFont* projectDefaultFont = wr->GetFontRegistry().EnsureDefaultProjectFont(18.0f, false))
			{
				io.FontDefault = projectDefaultFont;
				CH_CORE_INFO("RuntimeSystem: Switched default UI font to project font.");
			}
		}

		if (imguiLayer)
		{
			imguiLayer->RefreshFontAtlasTexture();
		}

		if (m_Scene)
		{
			Window& window = Application::Get().GetWindow();
			m_Scene->OnViewportResize(window.GetWidth(), window.GetHeight());
			EnsureRuntimeFramebuffer((uint32_t)window.GetWidth(), (uint32_t)window.GetHeight());
		}
	}

	void RuntimeLayer::OnDetach()
	{
		StopCurrentScene();

		m_Scene = nullptr;
		if (auto* se = ServiceLocator::TryGet<ScriptEngine>())
		{
			se->SetContextScene(nullptr);
		}
		m_LoadState = {};
	}

	void RuntimeLayer::OnUpdate(Timestep ts)
	{
		if (!m_PendingScenePath.empty())
		{
			std::string path = std::move(m_PendingScenePath);
			m_PendingScenePath.clear();
			LoadScene(path);
			return;
		}

		if (auto* uiRenderer = ServiceLocator::TryGet<WidgetRenderer>())
		{
			if (uiRenderer->GetFontRegistry().NeedsAtlasRebuild())
			{
				auto* imguiLayer = Application::Get().GetImGuiLayer();
				if (imguiLayer)
				{
					imguiLayer->ExecuteNextFrame([imguiLayer]() { imguiLayer->RefreshFontAtlasTexture(); });
				}
				uiRenderer->GetFontRegistry().ClearRebuildFlag();
			}
		}

		if (m_Scene && m_LoadState.State == RuntimeLoadState::LoadingScene)
		{
			m_LoadState.OverlayElapsed += (float)ts;

			if (IsSceneReadyToStart() && m_LoadState.OverlayElapsed >= m_LoadState.MinOverlayDuration)
			{
				if (auto* wr = ServiceLocator::TryGet<WidgetRenderer>())
				{
					wr->ResetButtonStates(m_Scene.get());
				}
				m_Scene->TransitionToState(SceneState::Play);
				m_LoadState.State = RuntimeLoadState::Running;
				m_LoadState.SuppressNextUIInput = true;
				CH_CORE_INFO("RuntimeSystem: Scene assets are ready, entering runtime.");
			}
		}

		if (m_Scene && IsRunning())
		{
			bool suppress = m_LoadState.SuppressNextUIInput;
			m_LoadState.SuppressNextUIInput = false;
			if (auto* wr = ServiceLocator::TryGet<WidgetRenderer>())
			{
				wr->ProcessInput(m_Scene.get(), suppress);
			}

			m_Scene->OnUpdateRuntime(ts);
		}

		if (m_LoadState.BoostUploadsTimer > 0.0f)
		{
			m_LoadState.BoostUploadsTimer -= ts;
		}
	}

	void RuntimeLayer::OnRender(Timestep ts)
	{
		Window& window = Application::Get().GetWindow();
		uint32_t width = (uint32_t)window.GetWidth();
		uint32_t height = (uint32_t)window.GetHeight();

		if (!m_Scene)
		{
			m_Renderer->Clear({0.0f, 0.0f, 0.0f, 1.0f});
			return;
		}

		if (width == 0 || height == 0)
		{
			return;
		}

		EnsureRuntimeFramebuffer(width, height);

		glm::vec4 bgColor = CalculateBackgroundColor();

		auto camConfig = GetCameraConfig();
		if (camConfig)
		{
			SceneRenderOptions options;

			m_HDRFramebuffer->Bind();
			m_Renderer->Clear(bgColor);
			m_SceneRenderer->RenderScene(m_Scene->GetRegistry(), m_Scene->GetSettings(), camConfig->Camera,
										 camConfig->NearClip, camConfig->FarClip, options);
			m_HDRFramebuffer->Unbind();
			m_HDRFramebuffer->Resolve();

			m_Renderer->SetViewport(0, 0, (int)width, (int)height);
			m_Renderer->Clear(bgColor);
			m_Renderer->ApplyPostProcessing(m_HDRFramebuffer->GetColorAttachmentRendererID(),
											m_HDRFramebuffer->GetDepthAttachmentRendererID(), camConfig->Camera,
											nullptr, {});
		}
		else
		{
			m_Renderer->Clear(bgColor);
		}
	}

	void RuntimeLayer::OnImGuiRender()
	{
		if (m_Scene)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
									 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
									 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus |
									 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

			if (ImGui::Begin("RuntimeUI", nullptr, flags))
			{
				if (IsRunning())
				{
					ImVec2 childSize = ImGui::GetContentRegionAvail();
					if (ImGui::BeginChild("##RuntimeUICanvas", childSize, false,
										  ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
											  ImGuiWindowFlags_NoScrollWithMouse))
					{
						ImVec2 canvasPos = ImGui::GetCursorScreenPos();
						ImVec2 canvasSize = ImGui::GetContentRegionAvail();
						if (auto* wr = ServiceLocator::TryGet<WidgetRenderer>())
						{
							wr->DrawCanvas(m_Scene.get(), canvasPos, canvasSize, false);
						}
						m_Scene->OnRenderUI();
					}
					ImGui::EndChild();
				}
			}
			ImGui::End();
			ImGui::PopStyleVar(2);

			if (m_LoadState.State == RuntimeLoadState::LoadingScene)
			{
				DrawLoadingOverlay();
			}
		}
	}

	void RuntimeLayer::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>([this](auto& ev) {
			if (ev.GetWidth() == 0 || ev.GetHeight() == 0)
			{
				return false;
			}

			EnsureRuntimeFramebuffer(ev.GetWidth(), ev.GetHeight());
			if (m_Scene)
			{
				m_Scene->OnViewportResize(ev.GetWidth(), ev.GetHeight());
			}
			return false;
		});

		dispatcher.Dispatch<SceneChangeRequestEvent>([this](auto& ev) {
			m_PendingScenePath = ev.GetPath();
			return true;
		});
	}

	//-----------------------------------------------------------------------------
	// Purpose: Load a new scene from file
	//-----------------------------------------------------------------------------
	void RuntimeLayer::LoadScene(const std::string& path)
	{
		const std::string normalizedPath = NormalizeAssetPath(path);
		if (normalizedPath.empty())
		{
			CH_CORE_WARN("RuntimeSystem: Ignoring empty scene path request.");
			return;
		}

		std::filesystem::path scenePath = normalizedPath;
		if (scenePath.is_relative() && Project::GetActive())
		{
			scenePath = Project::GetAssetPath(scenePath);
		}

		if (!scenePath.is_absolute())
		{
			std::error_code ec;
			scenePath = std::filesystem::absolute(scenePath, ec);
			if (ec)
			{
				CH_CORE_ERROR("RuntimeSystem: Failed to resolve absolute scene path '{}' ({})", normalizedPath,
							  ec.message());
				return;
			}
		}

		bool sceneAccessible = FileExists(scenePath);
		if (!sceneAccessible && m_AssetManager && m_AssetManager->IsPacked())
		{
			sceneAccessible = !m_AssetManager->ReadProjectAsset(scenePath).empty();
		}
		if (!sceneAccessible)
		{
			CH_CORE_ERROR("RuntimeSystem: Scene file not found '{}'.", scenePath.string());
			return;
		}

		if (!TransitionToScene(scenePath))
		{
			CH_CORE_ERROR("RuntimeSystem: Failed to transition to scene '{}'.", scenePath.string());
		}
	}

	bool RuntimeLayer::InitProject(const std::string& projectPath)
	{
		if (!DiscoverAndLoadProject(projectPath))
		{
			return false;
		}

		auto project = Project::GetActive();
		if (!project)
		{
			CH_CORE_ERROR("Runtime: No active project after DiscoverAndLoadProject - cannot initialize scripting.");
			return false;
		}
		auto assemblyPath =
			ScriptEngine::ResolveAssemblyPath(project->GetConfig().Scripting, project->GetProjectDirectoryForProject());

		CH_CORE_INFO("RuntimeSystem: Loading project assembly: {}", assemblyPath.string());

		auto* se = ServiceLocator::TryGet<ScriptEngine>();
		if (assemblyPath.empty() || !se || !se->ReloadAssembly(assemblyPath.string()))
		{
			CH_CORE_WARN(
				"RuntimeSystem: Script reload failed during project initialization (path: {}). Runtime continues "
				"without scripts.",
				assemblyPath.string());
		}

		if (auto* wr = ServiceLocator::TryGet<WidgetRenderer>())
		{
			wr->LoadProjectFonts();
		}

		ApplyWindowConfiguration();
		SetupBrandingAndIcon();

		LoadInitialScene();

		return true;
	}

	bool RuntimeLayer::DiscoverAndLoadProject(const std::string& projectPath)
	{
		std::filesystem::path discoveryPath = projectPath;

		if (discoveryPath.empty())
		{
			std::filesystem::path exePath = std::filesystem::absolute(
				std::filesystem::path(Application::Get().GetSpecification().CommandLineArgs.Args[0]));
			discoveryPath = exePath.parent_path();
		}

		if (discoveryPath.extension() == ".chproject")
		{
			m_ProjectPath = discoveryPath.string();
		}
		else
		{
			m_ProjectPath = (discoveryPath / (Application::Get().GetSpecification().Name + ".chproject")).string();
		}

		if (m_ProjectPath.empty())
		{
			return false;
		}

		std::filesystem::path packPath = Platform::GetExecutableDirectory() / "resources.pack";
		if (std::filesystem::exists(packPath))
		{
			m_AssetManager->OpenPack(packPath);
		}

		auto project = Project::Load(m_ProjectPath);
		if (!project)
		{
			CH_CORE_ERROR("RuntimeSystem: Failed to load project file at '{}'", m_ProjectPath);
			return false;
		}

		CH_CORE_INFO("RuntimeSystem: Project loaded: {}", project->GetName());
		CH_CORE_INFO("RuntimeSystem: Project Directory: {}", project->GetProjectDirectoryForProject().string());
		CH_CORE_INFO("RuntimeSystem: Asset Directory: {}", Project::GetAssetDirectory().string());

		m_AssetManager->SetProjectDirectory(project->GetProjectDirectoryForProject());
		m_AssetManager->SetAssetDirectory(Project::GetAssetDirectory());

#ifdef CH_SOURCE_GAME_DIR
		{
			std::filesystem::path srcGameDir(CH_SOURCE_GAME_DIR);
			std::filesystem::path srcAssetsDir = srcGameDir / "assets";
			if (std::filesystem::exists(srcAssetsDir))
			{
				CH_CORE_INFO("RuntimeSystem: Using source game assets: {}", srcAssetsDir.string());
				m_AssetManager->SetAssetDirectory(srcAssetsDir);
			}
		}
#endif

		m_Renderer->LoadEngineResources();

		if (auto* se = ServiceLocator::TryGet<ScriptEngine>())
		{
			se->TryAutoLoad(project->GetConfig());
		}

		return true;
	}

	void RuntimeLayer::ApplyWindowConfiguration()
	{
		auto project = Project::GetActive();
		if (!project)
		{
			return;
		}

		auto& config = project->GetConfig();
		Window& window = Application::Get().GetWindow();

		bool vsync = config.Window.VSync;
		int width = config.Window.Width;
		int height = config.Window.Height;
		bool fullscreen = config.Runtime.Fullscreen;

		const auto& args = Application::Get().GetSpecification().CommandLineArgs;
		for (int i = 1; i < args.Count; ++i)
		{
			std::string arg = args.Args[i];
			if (arg == "--width" && i + 1 < args.Count)
			{
				width = std::stoi(args.Args[++i]);
			}
			else if (arg == "--height" && i + 1 < args.Count)
			{
				height = std::stoi(args.Args[++i]);
			}
			else if (arg == "--fullscreen")
			{
				fullscreen = true;
			}
			else if (arg == "--windowed")
			{
				fullscreen = false;
			}
			else if (arg == "--vsync" && i + 1 < args.Count)
			{
				vsync = (std::string(args.Args[++i]) == "on");
			}
		}

		window.SetVSync(vsync);
		if (width != window.GetWidth() || height != window.GetHeight())
		{
			window.SetSize(width, height);
		}
		window.SetFullscreen(fullscreen);
	}

	void RuntimeLayer::SetupBrandingAndIcon()
	{
		auto project = Project::GetActive();
		if (!project)
		{
			return;
		}

		auto& config = project->GetConfig();
		Window& window = Application::Get().GetWindow();
		window.SetTitle(config.Name);

		if (config.IconPath.empty())
		{
			return;
		}

		// Try resolving via project path first (disk)
		std::string resolvedIconPath = project->GetAbsolutePathForProject(config.IconPath).string();
		if (std::filesystem::exists(resolvedIconPath))
		{
			CH_CORE_INFO("RuntimeSystem: Setting window icon: {}", resolvedIconPath);
			window.SetWindowIcon(resolvedIconPath);
			return;
		}

		// Try reading from pack (export mode)
		if (auto* am = ServiceLocator::TryGet<AssetManager>())
		{
			if (am->IsPacked())
			{
				auto data = am->ReadAssetData(config.IconPath);
				CH_CORE_INFO("RuntimeSystem: Icon pack lookup '{}' → {} bytes", config.IconPath, data.size());
				if (!data.empty())
				{
					CH_CORE_INFO("RuntimeSystem: Setting window icon from pack: {}", config.IconPath);
					window.SetWindowIconFromMemory(data.data(), data.size());
					return;
				}
			}
			else
			{
				CH_CORE_WARN("RuntimeSystem: AssetManager is NOT packed");
			}
		}

		CH_CORE_WARN("RuntimeSystem: Failed to resolve window icon: {}", config.IconPath);
	}

	void RuntimeLayer::LoadInitialScene()
	{
		auto project = Project::GetActive();
		if (!project)
		{
			return;
		}

		auto& config = project->GetConfig();
		std::string sceneToLoad = config.StartScene;

		const auto& args = Application::Get().GetSpecification().CommandLineArgs;
		for (int i = 1; i < args.Count; ++i)
		{
			if (std::string(args.Args[i]) == "--scene" && i + 1 < args.Count)
			{
				sceneToLoad = args.Args[++i];
				break;
			}
		}

		if (sceneToLoad.empty())
		{
			sceneToLoad = config.ActiveScenePath.string();
		}

		CH_CORE_INFO("RuntimeSystem: Initial scene to load: '{}'", sceneToLoad);

		if (sceneToLoad.empty())
		{
			std::filesystem::path scenesDir = Project::GetAssetDirectory() / "scenes";
			if (std::filesystem::exists(scenesDir))
			{
				try
				{
					for (const auto& entry : std::filesystem::recursive_directory_iterator(scenesDir))
					{
						if (entry.path().extension() == ".chscene")
						{
							sceneToLoad =
								std::filesystem::relative(entry.path(), Project::GetAssetDirectory()).string();
							break;
						}
					}
				} catch (const std::filesystem::filesystem_error& e)
				{
					CH_CORE_WARN("RuntimeSystem: Failed to enumerate scenes in '{}': {}", scenesDir.string(), e.what());
				}
			}
		}

		if (!sceneToLoad.empty())
		{
			LoadScene(sceneToLoad);
		}
	}

	void RuntimeLayer::StopCurrentScene()
	{
		if (!m_Scene)
		{
			return;
		}

		m_Scene->OnRuntimeStop();
	}

	void RuntimeLayer::AppendFontRequest(const TextStyle& style, std::vector<std::pair<std::string, float>>& out,
										 std::unordered_set<std::string>& dedupe) const
	{
		std::string fontName = NormalizeAssetPath(style.FontName);
		if (fontName.empty() || fontName == "Default")
		{
			return;
		}

		const float fontSize = (style.FontSize > 0.0f) ? style.FontSize : 16.0f;
		const int roundedHalf = static_cast<int>(std::lround(fontSize * 2.0f));
		const std::string key = fontName + "|" + std::to_string(roundedHalf);

		if (!dedupe.insert(key).second)
		{
			return;
		}

		out.emplace_back(fontName, fontSize);
	}

	std::vector<std::pair<std::string, float>> RuntimeLayer::CollectSceneFontRequests() const
	{
		std::vector<std::pair<std::string, float>> requests;
		if (!m_Scene)
		{
			return requests;
		}

		std::unordered_set<std::string> dedupe;
		auto& registry = m_Scene->GetRegistry();

		auto view = registry.view<UIControlComponent>();
		for (entt::entity id : view)
		{
			AppendFontRequest(view.get<UIControlComponent>(id).TextStyle, requests, dedupe);
		}

		return requests;
	}

	void RuntimeLayer::PreloadSceneFonts(bool allowRuntimeMutation)
	{
		auto requests = CollectSceneFontRequests();
		if (requests.empty())
		{
			return;
		}

		const int loadedCount = [&]() {
			if (auto* wr = ServiceLocator::TryGet<WidgetRenderer>())
			{
				return wr->GetFontRegistry().PreloadFonts(requests, allowRuntimeMutation);
			}
			return 0;
		}();
		if (loadedCount <= 0)
		{
			return;
		}

		CH_CORE_INFO("RuntimeSystem: Preloaded {} scene font tuple(s).", loadedCount);

		if (allowRuntimeMutation && ImGui::GetFrameCount() > 0)
		{
			if (auto* imguiLayer = Application::Get().GetImGuiLayer())
			{
				if (!imguiLayer->RefreshFontAtlasTexture())
				{
					CH_CORE_WARN("RuntimeSystem: Scene fonts were loaded, but font atlas refresh failed.");
				}
			}
		}
	}

	bool RuntimeLayer::SetupNewScene(const std::filesystem::path& scenePath)
	{
		auto nextScene = std::make_shared<Scene>();
		SceneSerializer serializer(nextScene.get());
		if (!serializer.Deserialize(scenePath.string()))
		{
			m_Scene = nullptr;
			if (auto* se = ServiceLocator::TryGet<ScriptEngine>())
			{
				se->SetContextScene(nullptr);
			}
			return false;
		}

		m_Scene = nextScene;
		m_Scene->GetSettings().ScenePath = scenePath.string();
		m_Scene->SetEventCallback([this](Event& e) { OnEvent(e); });

		if (auto* se = ServiceLocator::TryGet<ScriptEngine>())
		{
			se->SetContextScene(m_Scene.get());
		}

		Window& window = Application::Get().GetWindow();
		m_Scene->OnViewportResize(window.GetWidth(), window.GetHeight());
		EnsureRuntimeFramebuffer((uint32_t)window.GetWidth(), (uint32_t)window.GetHeight());

		return true;
	}

	void RuntimeLayer::ResetUIState()
	{
		if (!m_Scene)
		{
			return;
		}

		auto& registry = m_Scene->GetRegistry();
		auto view = registry.view<UIControlComponent>();
		for (entt::entity id : view)
		{
			view.get<UIControlComponent>(id).PressedThisFrame = false;
		}
	}

	void RuntimeLayer::BeginSceneLoading()
	{
		m_LoadState.SuppressNextUIInput = true;

		PreloadSceneFonts(ImGui::GetFrameCount() > 0);

		m_LoadState.BoostUploadsTimer = 5.0f;
		CH_CORE_INFO("RuntimeSystem: Boosting asset uploads for scene loading...");

		m_LoadState.State = RuntimeLoadState::LoadingScene;
		m_LoadState.OverlayElapsed = 0.0f;
		CH_CORE_INFO("RuntimeSystem: Scene loaded, waiting for async assets before runtime start.");
	}

	bool RuntimeLayer::TransitionToScene(const std::filesystem::path& scenePath)
	{
		StopCurrentScene();
		m_LoadState = {};

		if (!SetupNewScene(scenePath))
		{
			return false;
		}

		ResetUIState();
		BeginSceneLoading();
		return true;
	}

	std::optional<RuntimeLayer::CameraConfig> RuntimeLayer::GetCameraConfig()
	{
		auto camera = GetActiveCamera();
		if (!camera)
		{
			return std::nullopt;
		}

		CameraConfig activeCameraConfig;
		activeCameraConfig.Camera = camera.value();

		Entity primaryCam = SceneRenderer::GetPrimaryCameraEntity(m_Scene->GetRegistry(), m_Scene->GetRegistryPtr());
		if (primaryCam && primaryCam.HasComponent<CameraComponent>())
		{
			auto& cameraComp = primaryCam.GetComponent<CameraComponent>().Camera;
			activeCameraConfig.NearClip = cameraComp.GetPerspectiveNearClip();
			activeCameraConfig.FarClip = cameraComp.GetPerspectiveFarClip();
		}

		return activeCameraConfig;
	}

	glm::vec4 RuntimeLayer::CalculateBackgroundColor() const
	{
		const auto& settings = m_Scene->GetSettings();
		glm::vec4 bgColor = {settings.BackgroundColor.r / 255.0f, settings.BackgroundColor.g / 255.0f,
							 settings.BackgroundColor.b / 255.0f, settings.BackgroundColor.a / 255.0f};

		if (settings.Environment && settings.Mode != BackgroundMode::Color)
		{
			auto& env = settings.Environment->GetSettings();
			if (env.Fog.Enabled)
			{
				bgColor = glm::vec4(env.Fog.FogColor.r / 255.0f, env.Fog.FogColor.g / 255.0f,
									env.Fog.FogColor.b / 255.0f, env.Fog.FogColor.a / 255.0f);
			}
		}

		return bgColor;
	}

	std::optional<Camera3D> RuntimeLayer::GetActiveCamera()
	{
		if (m_Scene)
		{
			return SceneRenderer::GetActiveCamera(m_Scene->GetRegistry());
		}
		return std::nullopt;
	}

	void RuntimeLayer::EnsureRuntimeFramebuffer(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			return;
		}

		auto project = Project::GetActive();
		int msaaSampleCount = project ? project->GetConfig().Render.AntiAliasingSamples : 4;
		uint32_t msaaSamplesClamped = msaaSampleCount > 1 ? (uint32_t)msaaSampleCount : 1u;

		if (m_HDRFramebuffer && msaaSamplesClamped != m_MSAAFramebufferSamples)
		{
			m_HDRFramebuffer.reset();
		}

		if (!m_HDRFramebuffer)
		{
			FramebufferSpecification hdrSpec;
			hdrSpec.Width = width;
			hdrSpec.Height = height;
			hdrSpec.Samples = msaaSamplesClamped;
			hdrSpec.ColorFormat = FramebufferColorFormat::RGBA16F;
			m_HDRFramebuffer = Framebuffer::Create(hdrSpec);
			m_MSAAFramebufferSamples = msaaSamplesClamped;
		}
		else
		{
			const auto& spec = m_HDRFramebuffer->GetSpecification();
			if (spec.Width != width || spec.Height != height)
			{
				m_HDRFramebuffer->Resize(width, height);
			}
		}
	}

	bool RuntimeLayer::IsSceneReadyToStart() const
	{
		auto* assetManager = m_AssetManager;
		if (!assetManager)
		{
			return true;
		}
		return !assetManager->HasBackgroundWork();
	}

	void RuntimeLayer::DrawLoadingOverlay()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
								 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
								 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.02f, 0.92f));

		if (ImGui::Begin("##RuntimeLoadingOverlay", nullptr, flags))
		{
			const size_t totalPending = m_AssetManager->GetLoadingAssetCount();

			int dotsCount = (static_cast<int>(ImGui::GetTime() * 2.5f) % 3) + 1;
			std::string dots(static_cast<size_t>(dotsCount), '.');
			std::string loadingLine = "Preparing world" + dots;
			std::string pendingLine = "Pending assets: " + std::to_string(totalPending);

			ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.45f);

			const char* title = "Loading scene";
			ImVec2 titleSize = ImGui::CalcTextSize(title);
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
			ImGui::TextUnformatted(title);

			ImVec2 loadingSize = ImGui::CalcTextSize(loadingLine.c_str());
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - loadingSize.x) * 0.5f);
			ImGui::TextUnformatted(loadingLine.c_str());

			ImVec2 pendingSize = ImGui::CalcTextSize(pendingLine.c_str());
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pendingSize.x) * 0.5f);
			ImGui::TextUnformatted(pendingLine.c_str());
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}
} // namespace Chained
