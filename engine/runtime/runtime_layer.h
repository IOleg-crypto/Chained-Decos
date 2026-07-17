#ifndef CH_RUNTIME_LAYER_H
#define CH_RUNTIME_LAYER_H

#include "engine/core/layer.h"
#include "engine/graphics/api/framebuffer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_context.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/assets/asset_manager.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Chained
{
// Runs the game/runtime experience, including scene loading and the scene renderer.
class RuntimeLayer : public Layer
{
public:
    RuntimeLayer(const std::string& projectPath);

    ~RuntimeLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Timestep ts) override;
    void OnRender(Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(Event& e) override;

    // Loads a scene by project-relative or absolute path.
    void LoadScene(const std::string& path);
    // Loads a scene by index from the active project configuration.
    void LoadScene(int index);

private:
    bool InitProject(const std::string& projectPath);
    bool DiscoverAndLoadProject(const std::string& projectPath);
    void ApplyWindowConfiguration();
    void SetupBrandingAndIcon();
    void LoadInitialScene();
    std::string NormalizeScenePath(const std::string& path) const;
    bool TransitionToScene(const std::filesystem::path& scenePath);
    void StopCurrentScene();
    void PreloadSceneFonts(bool allowRuntimeMutation);
    std::vector<std::pair<std::string, float>> CollectSceneFontRequests() const;
    void EnsureRuntimeFramebuffer(uint32_t width, uint32_t height);
    bool IsSceneReadyToStart() const;
    void DrawLoadingOverlay();

    std::optional<Camera3D> GetActiveCamera();

private:
    std::shared_ptr<Scene> m_Scene;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    // Resolved once in the constructor (ServiceLocator is already locked by then —
    // see Application::Application) and reused for the layer's whole lifetime.
    SceneContext m_Context;
    Renderer* m_Renderer = nullptr;
    AssetManager* m_AssetManager = nullptr;
private:

    std::string m_ProjectPath;
    float m_BoostUploadsTimer = 0.0f;
    bool m_IsBoostingUploads = false;
    bool m_RuntimeStarted = false;
    bool m_IsSceneLoading = false;
    float m_LoadingOverlayElapsed = 0.0f;
    float m_LoadingOverlayMinDuration = 0.35f;

    std::string m_PendingScenePath;
    std::shared_ptr<Framebuffer> m_HDRFramebuffer;
    uint32_t m_HDRFramebufferSamples = 0;
    bool m_SuppressNextUIInput = false;
};
} // namespace Chained

#endif // CH_RUNTIME_LAYER_H
