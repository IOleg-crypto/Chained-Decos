#ifndef CD_RUNTIME_RUNTIMELAYER_H
#define CD_RUNTIME_RUNTIMELAYER_H

#include "engine/core/layer/layer.h"
#include "engine/scene/core/scene.h"
#include <memory>

#include <raylib.h>

namespace CHD
{

class RuntimeLayer : public CHEngine::Layer
{
public:
    RuntimeLayer(std::shared_ptr<CHEngine::Scene> scene);
    virtual ~RuntimeLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender() override;
    virtual void OnEvent(CHEngine::Event &event) override;

    void RenderScene();
    void RenderUI(float width, float height);

private:
    Font m_hudFont;
    bool m_fontLoaded = false;

    // Player Shader
    Shader m_playerShader = {0};
    int m_locFallSpeed = -1;
    int m_locTime = -1;
    bool m_shaderLoaded = false;
    std::shared_ptr<CHEngine::Scene> m_Scene;
};

} // namespace CHD

#endif // CD_RUNTIME_RUNTIMELAYER_H
