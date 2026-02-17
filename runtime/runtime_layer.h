#ifndef CH_RUNTIME_LAYER_H
#define CH_RUNTIME_LAYER_H

#include "engine/core/layer.h"
#include "engine/scene/scene.h"
#include <functional>
#include <memory>
#include <string>
#include <optional>
#include "raylib.h"

namespace CHEngine
{
    class RuntimeLayer : public Layer
    {
    public:
        using ScriptRegistrationCallback = std::function<void(Scene*)>;

        RuntimeLayer(const std::string& projectPath = "", ScriptRegistrationCallback callback = nullptr);
        virtual ~RuntimeLayer();

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(Timestep ts) override;
        virtual void OnRender(Timestep ts) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Event &e) override;

        void LoadScene(const std::string& path);
        void LoadScene(int index);

    private:
        bool InitProject(const std::string& projectPath);
        bool DiscoverAndLoadProject(const std::string& projectPath);
        void ApplyWindowConfiguration();
        void SetupBrandingAndIcon();
        void LoadInitialScene();

        std::optional<Camera3D> GetActiveCamera();

    private:
        std::shared_ptr<Scene> m_Scene;
        std::unique_ptr<class SceneRenderer> m_SceneRenderer;
        ScriptRegistrationCallback m_ScriptCallback;
        std::string m_PendingScenePath;
        std::string m_ProjectPath;
    };
}

#endif // CH_RUNTIME_LAYER_H
