#include "runtime_application.h"
#include "engine/core/application.h"
#include "engine/core/window.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/renderer.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scene_events.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/ui_renderer.h"
#include "engine/scene/scene_scripting.h"
#include "engine/core/module_loader.h"
#include "imgui.h"
#include "raymath.h"
#include <filesystem>

namespace CHEngine
{
class RuntimeLayer : public Layer
{
public:
    RuntimeLayer(RuntimeApplication::ScriptRegistrationCallback callback) 
        : Layer("RuntimeLayer"), m_ScriptCallback(callback)
    {
        m_SceneRenderer = std::make_unique<SceneRenderer>();
    }

    ~RuntimeLayer()
    {
    }

    virtual void OnUpdate(Timestep ts) override
    {
        // Process deferred scene transition (safe - not inside scene update)
        if (!m_PendingScenePath.empty())
        {
            std::string path = m_PendingScenePath;
            m_PendingScenePath.clear();
            LoadScene(path);
        }

        if (m_Scene)
            m_Scene->OnUpdateRuntime(ts);
    }

    virtual void OnRender(Timestep ts) override
    {
        if (!m_Scene)
        {
            ::ClearBackground(BLACK);
            return;
        }

        // Clear background with fog color or black
        Color bgColor = BLACK;
        if (m_Scene->GetSettings().Environment)
        {
            auto& env = m_Scene->GetSettings().Environment->GetSettings();
            if (env.Fog.Enabled) bgColor = env.Fog.FogColor;
        }
        ::ClearBackground(bgColor);

        auto camera = GetActiveCamera();
        m_SceneRenderer->RenderScene(m_Scene.get(), camera, ts);
    }

    virtual void OnImGuiRender() override
    {
        if (m_Scene)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | 
                                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | 
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | 
                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
                                     ImGuiWindowFlags_NoDocking;

            flags &= ~ImGuiWindowFlags_NoInputs; // Allow inputs

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            
            if (ImGui::Begin("RuntimeUI", nullptr, flags))
            {
                // Use ImGui's content area coordinates — same as editor viewport
                ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                UIRenderer::Get().DrawCanvas(m_Scene.get(), canvasPos, canvasSize, false);
                SceneScripting::RenderUI(m_Scene.get());
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }
    }

    virtual void OnEvent(Event &e) override
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<SceneChangeRequestEvent>([this](auto& ev) {
            // Defer scene load to next frame to avoid destroying scene mid-update
            m_PendingScenePath = ev.GetPath();
            return true;
        });

        if (m_Scene)
            m_Scene->OnEvent(e);
    }

    void LoadScene(const std::string& path)
    {
        std::filesystem::path scenePath = path;
        // If the path is relative, resolve it via Project::GetAssetPath
        if (scenePath.is_relative() && Project::GetActive())
        {
            scenePath = Project::GetAssetPath(path);
        }
        
        std::string finalPath = scenePath.string();

        if (m_Scene)
            m_Scene->OnRuntimeStop();

        m_Scene = std::make_shared<Scene>();
        
        // 1. Register Scripts BEFORE deserialization
        if (m_ScriptCallback) {
            m_ScriptCallback(m_Scene.get());
            CH_CORE_INFO("Runtime: Registered scripts via static callback for: {}", finalPath);
        } else {
            // Attempt to load game module dynamically
            auto project = Project::GetActive();
            
            if (project && project->GetConfig().Scripting.AutoLoad)
            {
                const auto& scripting = project->GetConfig().Scripting;
                std::string moduleName = scripting.ModuleName;
                if (moduleName.empty())
                    moduleName = project->GetConfig().Name + "game";

                #ifdef CH_PLATFORM_WINDOWS
                std::string dllName = moduleName + ".dll";
                #else
                std::string dllName = "lib" + moduleName + ".so";
                #endif

                std::filesystem::path binDir = std::filesystem::path(Application::Get().GetSpecification().CommandLineArgs.Args[0]).parent_path();
                std::filesystem::path projectDir = project->GetProjectDirectory();
                
                std::vector<std::filesystem::path> searchPaths;
                
                // 1. Explicit ModuleDirectory (relative to project)
                if (!scripting.ModuleDirectory.empty())
                {
                    if (scripting.ModuleDirectory.is_absolute())
                        searchPaths.push_back(scripting.ModuleDirectory / dllName);
                    else
                        searchPaths.push_back(projectDir / scripting.ModuleDirectory / dllName);
                }

                // 2. Project Root / bin
                searchPaths.push_back(projectDir / "bin" / dllName);
                searchPaths.push_back(projectDir / dllName);

                // 3. Executable Directory
                searchPaths.push_back(binDir / dllName);

                bool loaded = false;
                for (const auto& path : searchPaths)
                {
                    if (std::filesystem::exists(path))
                    {
                        if (ModuleLoader::LoadGameModule(path.string(), &m_Scene->GetScriptRegistry()))
                        {
                            CH_CORE_INFO("Runtime: Loaded scripts from: {}", path.string());
                            loaded = true;
                            break;
                        }
                    }
                }

                if (!loaded)
                {
                    CH_CORE_WARN("Runtime: Failed to find or load game module '{}'. Checked {} locations.", moduleName, searchPaths.size());
                }
            }
        }

        // 2. Deserialize scene (NativeScriptComponent will now find registered factories)
        SceneSerializer serializer(m_Scene.get());
        if (serializer.Deserialize(finalPath))
        {
             m_Scene->GetSettings().ScenePath = finalPath;
             
             // 3. Start runtime AFTER everything is loaded and registered
             m_Scene->OnRuntimeStart();
        }
        else
        {
            CH_CORE_ERROR("Runtime: Failed to load scene: {}", finalPath);
            m_Scene = nullptr;
        }
    }

    Camera3D GetActiveCamera()
    {
        if (m_Scene)
            return m_Scene->GetActiveCamera();

        // Fallback
        Camera3D camera = {0};
        camera.position = {10.0f, 10.0f, 10.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }

private:
    std::shared_ptr<Scene> m_Scene;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    RuntimeApplication::ScriptRegistrationCallback m_ScriptCallback;
    std::string m_PendingScenePath;
};


    RuntimeApplication::RuntimeApplication(const ApplicationSpecification& specification,
                                           const std::string& projectPath,
                                           ScriptRegistrationCallback scriptCallback)
        : Application(specification),
          m_ProjectPath(projectPath),
          m_ScriptCallback(scriptCallback)
    {
        m_RuntimeLayer = new RuntimeLayer(m_ScriptCallback);
        PushLayer(m_RuntimeLayer);

        if (InitProject(projectPath))
        {
            if (LoadModule())
            {
                // Initial scene loading is handled by InitProject as part of bootstrap
            }
        }
    }

    bool RuntimeApplication::InitProject(const std::string& projectPath)
    {
        // 1. Discovery & Loading
        std::filesystem::path discoveryPath = projectPath;
        if (discoveryPath.empty())
        {
            std::filesystem::path exePath = std::filesystem::absolute(std::filesystem::path(GetSpecification().CommandLineArgs.Args[0]));
            discoveryPath = exePath.parent_path();
        }

        // Use Speicification name (App Title) as discovery hint
        m_ProjectPath = Project::Discover(discoveryPath, GetSpecification().Name).string();
        
        if (m_ProjectPath.empty())
        {
            CH_CORE_ERROR("Runtime: No project file found!");
            return false;
        }

        auto project = Project::Load(m_ProjectPath);
        if (!project)
        {
            CH_CORE_ERROR("Runtime: Failed to load project: {}", m_ProjectPath);
            return false;
        }

        // 2. Window Setup & CLI Overrides
        auto &config = project->GetConfig();
        Window& window = GetWindow();
        
        // Default from config
        bool vsync = config.Window.VSync;
        int width = config.Window.Width;
        int height = config.Window.Height;
        bool fullscreen = config.Runtime.Fullscreen;

        // CLI Overrides (Priority: CLI > Config)
        const auto& args = GetSpecification().CommandLineArgs;
        for (int i = 1; i < args.Count; ++i)
        {
            std::string arg = args.Args[i];
            if (arg == "--width" && i + 1 < args.Count) width = std::stoi(args.Args[++i]);
            else if (arg == "--height" && i + 1 < args.Count) height = std::stoi(args.Args[++i]);
            else if (arg == "--fullscreen") fullscreen = true;
            else if (arg == "--windowed") fullscreen = false;
            else if (arg == "--vsync" && i + 1 < args.Count) vsync = (std::string(args.Args[++i]) == "on");
        }

        window.SetVSync(vsync);
        if (width != config.Window.Width || height != config.Window.Height)
        {
            window.SetSize(width, height);
        }
        window.SetFullscreen(fullscreen);

        // Dynamic Branding: Name & Icon
        window.SetTitle(config.Name);

        std::filesystem::path iconPath = "";
        if (!config.IconPath.empty())
        {
            CH_CORE_INFO("Icon: Trying to resolve '{}'", config.IconPath);

            // Try resolving via AssetManager search paths
            std::string resolved = project->GetAssetManager()->ResolvePath(config.IconPath);
            if (std::filesystem::exists(resolved))
            {
                iconPath = resolved;
                CH_CORE_INFO("Icon: Found via AssetManager: {}", resolved);
            }
            else
            {
                CH_CORE_WARN("Icon: AssetManager resolved to '{}' but it doesn't exist", resolved);
                
                // Try relative to project directory
                std::filesystem::path projectRelative = project->GetProjectDirectory() / config.IconPath;
                if (std::filesystem::exists(projectRelative))
                {
                    iconPath = projectRelative;
                    CH_CORE_INFO("Icon: Found relative to project: {}", projectRelative.string());
                }
                // Try relative to engine root
                else if (!Project::GetEngineRoot().empty())
                {
                    std::filesystem::path engineRelative = Project::GetEngineRoot() / config.IconPath;
                    if (std::filesystem::exists(engineRelative))
                    {
                        iconPath = engineRelative;
                        CH_CORE_INFO("Icon: Found relative to engine root: {}", engineRelative.string());
                    }
                    else
                    {
                        CH_CORE_WARN("Icon: Not found at engine root: {}", engineRelative.string());
                    }
                }
                
                // Try relative to executable directory
                if (iconPath.empty())
                {
                    std::filesystem::path exeDir = std::filesystem::path(
                        Application::Get().GetSpecification().CommandLineArgs.Args[0]).parent_path();
                    std::filesystem::path exeRelative = exeDir / config.IconPath;
                    if (std::filesystem::exists(exeRelative))
                    {
                        iconPath = exeRelative;
                        CH_CORE_INFO("Icon: Found relative to exe: {}", exeRelative.string());
                    }
                }
            }
        }

        // Fallback icon discovery
        if (iconPath.empty() || !std::filesystem::exists(iconPath))
        {
             std::vector<std::string> iconNames = {"icon.png", "icon.jpg", "assets/icon.png"};
             for (const auto& name : iconNames)
             {
                 std::filesystem::path p = project->GetProjectDirectory() / name;
                 if (std::filesystem::exists(p))
                 {
                     iconPath = p;
                     break;
                 }
             }
             if (iconPath.empty())
                 CH_CORE_WARN("Icon: No icon found (config='{}', engineRoot='{}')", 
                     config.IconPath, Project::GetEngineRoot().string());
        }

        if (std::filesystem::exists(iconPath))
        {
            Image icon = LoadImage(iconPath.string().c_str());
            
            if (icon.data != nullptr)
            {
                // GLFW requires RGBA format for window icons
                if (icon.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
                    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                
                window.SetWindowIcon(icon);
                CH_CORE_INFO("Loaded icon {}" , iconPath.string());
                UnloadImage(icon);
            }
        }
        
        // FPS setup (from config)
        ::SetTargetFPS((int)config.Animation.TargetFPS > 0 ? (int)config.Animation.TargetFPS : 60); 

        // 3. Initial Scene Load
        std::string sceneToLoad = config.StartScene;
        
        // CLI Scene Override
        for (int i = 1; i < args.Count; ++i)
        {
            if (std::string(args.Args[i]) == "--scene" && i + 1 < args.Count)
            {
                sceneToLoad = args.Args[++i];
                break;
            }
        }

        if (sceneToLoad.empty()) sceneToLoad = config.ActiveScenePath.string();

        if (sceneToLoad.empty())
        {
            // Final fallback: any scene
            std::filesystem::path scenesDir = Project::GetAssetDirectory() / "scenes";
            if (std::filesystem::exists(scenesDir))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(scenesDir))
                {
                    if (entry.path().extension() == ".chscene")
                    {
                        sceneToLoad = std::filesystem::relative(entry.path(), Project::GetAssetDirectory()).string();
                        break;
                    }
                }
            }
        }

        if (!sceneToLoad.empty())
        {
            std::filesystem::path fullPath = Project::GetAssetPath(sceneToLoad);
            CH_CORE_INFO("Runtime: Loading start scene: {}", fullPath.string());
            LoadScene(fullPath.string());
        }

        return true;
    }

    bool RuntimeApplication::LoadModule()
    {
        // For now Module loading is tied to Scene loading in RuntimeLayer::LoadScene
        return true;
    }

    void RuntimeApplication::LoadScene(const std::string& scenePath)
    {
        if (m_RuntimeLayer)
            ((RuntimeLayer*)m_RuntimeLayer)->LoadScene(scenePath);
    }

    void RuntimeApplication::LoadScene(int index)
    {
        auto project = Project::GetActive();
        if (!project) return;
        
        const auto& buildScenes = project->GetConfig().BuildScenes;
        if (index >= 0 && index < buildScenes.size())
        {
            std::filesystem::path fullPath = Project::GetAssetPath(buildScenes[index]);
            LoadScene(fullPath.string());
        }
        else
        {
            CH_CORE_ERROR("Runtime: Invalid scene index {} (BuildScenes count: {})", index, buildScenes.size());
        }
    }

} // namespace CHEngine
