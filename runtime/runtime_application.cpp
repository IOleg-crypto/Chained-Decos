#include "runtime_application.h"
#include "engine/core/application.h"
#include "engine/core/window.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
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

    virtual void OnUpdate(float deltaTime) override
    {
        auto scene = Application::Get().GetActiveScene();
        if (scene)
            scene->OnUpdateRuntime(deltaTime);
    }

    virtual void OnRender() override
    {
        auto activeScene = Application::Get().GetActiveScene();
        if (!activeScene)
            return;

        auto camera = GetActiveCamera();
        activeScene->OnRender(camera);
    }

    virtual void OnImGuiRender() override
    {
        auto activeScene = Application::Get().GetActiveScene();
        if (activeScene)
        {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            
            // Create a fullscreen background window for the UI
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | 
                                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | 
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | 
                                     ImGuiWindowFlags_NoBringToFrontOnFocus;

            // Note: We want inputs to pass through to the game, but UI elements need to capture mouse.
            // ImGuiWindowFlags_NoInputs prevents ANY interaction, so we should NOT use it if we want buttons to work.
            // Instead, we use NoBackground and make it full screen.
            flags &= ~ImGuiWindowFlags_NoInputs; // Allow inputs

            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize(displaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            
            if (ImGui::Begin("RuntimeUI", nullptr, flags))
            {
                activeScene->OnImGuiRender({0, 0}, displaySize, 0, false);
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }
    }

    Camera3D GetActiveCamera()
    {
        auto activeScene = Application::Get().GetActiveScene();
        if (activeScene)
            return activeScene->GetActiveCamera();

        // Fallback
        Camera3D camera = {0};
        camera.position = {10.0f, 10.0f, 10.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }
};


    RuntimeApplication::RuntimeApplication(const Application::Config &config,
                                           const std::string &projectPath)
        : Application(([&config]() {
              Application::Config c = config;
              c.EnableViewports = false;
              c.EnableDocking = false;
              return c;
          })()),
          m_ProjectPath(projectPath)
    {
        // For standalone, try to find the project root relative to the executable
        std::filesystem::path exePath = std::filesystem::absolute(std::filesystem::path(config.Argv[0]));
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
                AssetManager::SetRootPath(current);
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

        PushLayer(new RuntimeLayer());
    }

void RuntimeApplication::PostInitialize()
{
    if (!m_ProjectPath.empty())
    {
        auto project = Project::Load(m_ProjectPath);
        if (project)
        {
            // Apply Project Settings
            auto &config = project->GetConfig();

            // Note: Window size is usually set in CreateApplication, but we can sync VSync/FPS here
            if (auto &window = GetWindow())
            {
                window->SetVSync(config.Window.VSync);
                
                // Set window icon if available
                std::filesystem::path iconPath = AssetManager::ResolvePath("engine:icons/chaineddecos.jpg");
                if (std::filesystem::exists(iconPath))
                {
                    Image icon = LoadImage(iconPath.string().c_str());
                    if (icon.data != nullptr)
                    {
                        window->SetWindowIcon(icon);
                        UnloadImage(icon);
                    }
                }
            }
            SetTargetFPS(60); 

            std::string sceneToLoad = config.StartScene;
            if (sceneToLoad.empty())
            {
                sceneToLoad = config.ActiveScenePath.string();
            }

            // FALLBACK: If no scene specified in project, try to find any scene in assets/scenes
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
                // Resolve the path relative to the project assets
                std::filesystem::path fullPath = Project::GetAssetPath(sceneToLoad);
                CH_CORE_INFO("Standalone: Loading start scene: {}", fullPath.string());
                LoadScene(fullPath.string());

                auto activeScene = GetActiveScene();
                if (activeScene && project->GetEnvironment())
                {
                    if (activeScene->GetEnvironment()->GetPath().empty() && 
                        activeScene->GetSkybox().TexturePath.empty())
                    {
                        activeScene->SetEnvironment(project->GetEnvironment());
                        CH_CORE_INFO("Standalone: Applied project environment to scene");
                    }
                }

                if (activeScene)
                {
                    activeScene->OnRuntimeStart();
                }
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
