#include "runtime_application.h"
#include "engine/core/application.h"
#include "engine/core/window.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/render.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scene_events.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/ui_renderer.h"
#include "imgui.h"
#include "raymath.h"
#include <filesystem>

namespace CHEngine
{
class RuntimeLayer : public Layer
{
public:
    RuntimeLayer() : Layer("RuntimeLayer")
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
            return;

        auto camera = GetActiveCamera();
        SceneRenderer::RenderScene(m_Scene.get(), camera, ts);
    }

    virtual void OnImGuiRender() override
    {
        if (m_Scene)
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            
            // Create a fullscreen background window for the UI
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | 
                                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | 
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | 
                                     ImGuiWindowFlags_NoBringToFrontOnFocus;

            flags &= ~ImGuiWindowFlags_NoInputs; // Allow inputs

            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize(displaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            
            if (ImGui::Begin("RuntimeUI", nullptr, flags))
            {
                UIRenderer::DrawCanvas(m_Scene.get(), {0, 0}, displaySize, false);
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
        if (m_Scene)
            m_Scene->OnRuntimeStop();

        m_Scene = std::make_shared<Scene>();
        SceneSerializer serializer(m_Scene.get());
        if (serializer.Deserialize(path))
        {
             m_Scene->GetSettings().ScenePath = path;
             
             // Apply project environment if needed
             if (Project::GetActive() && Project::GetActive()->GetEnvironment())
             {
                 if (m_Scene->GetSettings().Environment->GetPath().empty() && 
                     m_Scene->GetSettings().Environment->GetSettings().Skybox.TexturePath.empty())
                 {
                     m_Scene->GetSettings().Environment = Project::GetActive()->GetEnvironment();
                 }
             }
             
             m_Scene->OnRuntimeStart();
        }
        else
        {
            CH_CORE_ERROR("Runtime: Failed to load scene: {}", path);
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
};


    RuntimeApplication::RuntimeApplication(const ApplicationSpecification& specification,
                                           const std::string& projectPath)
        : Application(specification),
          m_ProjectPath(projectPath)
    {
        // For standalone, try to find the project root relative to the executable
        std::filesystem::path exePath = std::filesystem::absolute(std::filesystem::path(specification.CommandLineArgs.Args[0]));
        std::filesystem::path current = exePath.parent_path();
        
        bool projectFound = false;
        while (current.has_parent_path())
        {
            // Search for project file if not yet found
            if (!projectFound)
            {
                std::error_code ec;
                // 1. Try immediate parent directory (typical for production distribution)
                if (std::filesystem::exists(current, ec))
                {
                    for (const auto& entry : std::filesystem::directory_iterator(current, ec))
                    {
                        if (entry.path().extension() == ".chproject")
                        {
                            m_ProjectPath = entry.path().string();
                            CH_CORE_INFO("Standalone: Auto-discovered project file: {}", m_ProjectPath);
                            projectFound = true;
                            break;
                        }
                    }
                }

                // 2. Try searching subdirectories (useful for dev environments like build/bin/ or root/)
                if (!projectFound)
                {
                    auto it = std::filesystem::recursive_directory_iterator(current, std::filesystem::directory_options::skip_permission_denied, ec);
                    auto end = std::filesystem::recursive_directory_iterator();
                    
                    while (it != end)
                    {
                        if (ec) 
                        { 
                            ec.clear(); 
                            it.disable_recursion_pending(); 
                            if (++it == end) break;
                            continue; 
                        }
                        
                        // Limit depth to avoid walking the whole drive
                        if (it.depth() > 2) 
                        {
                            it.disable_recursion_pending();
                            if (++it == end) break;
                            continue;
                        }

                        if (it->path().extension() == ".chproject")
                        {
                            m_ProjectPath = it->path().string();
                            CH_CORE_INFO("Standalone: Auto-discovered project file in subdirectory: {}", m_ProjectPath);
                            projectFound = true;
                            break;
                        }
                        
                        if (++it == end) break;
                    }
                }
            }

            // Engine assets discovery (shaders, default fonts)
            if (std::filesystem::exists(current / "engine/resources"))
            {
                // AssetManager::SetRootPath(current); // Removed: AssetManager is now per-project
                // If we also found the project, we can stop here
                if (projectFound)
                {
                    break;
                }
            }
            
            // If we've already found a project file, we might be done, 
            // but we still want to find the engine root (engine/resources) if possible.
            // If we just hit the root and found nothing new, break.
            current = current.parent_path();
        }

        m_RuntimeLayer = new RuntimeLayer();
        PushLayer(m_RuntimeLayer);

        // --- PostInitialize logic moved here ---
        if (!m_ProjectPath.empty())
        {
            auto project = Project::Load(m_ProjectPath);
            if (project)
            {
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

                std::string sceneToLoad = config.StartScene;
                if (sceneToLoad.empty()) sceneToLoad = config.StartScene;
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
                                CH_CORE_INFO("Standalone: Start scene not set. Falling back to: {}", sceneToLoad);
                                break;
                            }
                        }
                    }
                }

                if (!sceneToLoad.empty())
                {
                    std::filesystem::path fullPath = Project::GetAssetPath(sceneToLoad);
                    CH_CORE_INFO("Standalone: Loading start scene: {}", fullPath.string());
                    ((RuntimeLayer*)m_RuntimeLayer)->LoadScene(fullPath.string());
                }
                else
                {
                    CH_CORE_ERROR("Standalone: No scene found to load!");
                }
            }
        }
        else
        {
            CH_CORE_ERROR("Standalone: No project file found!");
        }
    }

} // namespace CHEngine
