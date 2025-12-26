#ifndef RUNTIMELAYER_H
#define RUNTIMELAYER_H

#include "core/layer/Layer.h"

#include <raylib.h>

namespace CHD
{

class RuntimeLayer : public CHEngine::Layer
{
public:
    RuntimeLayer();
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
};

} // namespace CHD

#endif // RUNTIMELAYER_H
