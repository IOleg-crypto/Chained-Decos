#ifndef GAMELAYER_H
#define GAMELAYER_H

#include "core/layer/Layer.h"
#include <raylib.h>

class GameLayer : public ChainedDecos::Layer
{
public:
    GameLayer();
    virtual ~GameLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender() override;
    virtual void OnEvent(ChainedDecos::Event &event) override;

    void RenderScene();
    void RenderUI(float width, float height);

private:
    Font m_hudFont;
    bool m_fontLoaded = false;
};

#endif // GAMELAYER_H
