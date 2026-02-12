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
                UIRenderer::Get().DrawCanvas(m_Scene.get(), {0, 0}, viewport->WorkSize, false);
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
            LoadScene(ev.GetPath());
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
        } else {
            CH_CORE_WARN("Runtime: No script registration callback provided!");
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

        m_ProjectPath = Project::Discover(discoveryPath).string();
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

        // Re-initialize Render system with the now-active project for engine resources (shaders, icons)
        // Renderer::Init(); // This is a singleton, so we should check if it's already init.
        // For now, let's just make sure we are not calling a static Initialize that doesn't exist.
        if (Renderer::Get().GetData().LightingShader == nullptr) {
             // If not fully setup, maybe call Init, but Application usually does it.
        }

        // 2. Window Setup
        auto &config = project->GetConfig();
        Window& window = GetWindow();
        window.SetVSync(config.Window.VSync);
        
        std::filesystem::path iconPath = project->GetAssetManager()->ResolvePath("engine/resources/icons/chaineddecos.jpg");
        if (std::filesystem::exists(iconPath))
        {
            Image icon = LoadImage(iconPath.string().c_str());
            if (icon.data != nullptr)
            {
                window.SetWindowIcon(icon);
                UnloadImage(icon);
            }
        }
        ::SetTargetFPS(60); 

        // 3. Initial Scene Load
        std::string sceneToLoad = config.StartScene;
        if (sceneToLoad.empty()) sceneToLoad = config.ActiveScenePath.string();

        if (sceneToLoad.empty())
        {
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
