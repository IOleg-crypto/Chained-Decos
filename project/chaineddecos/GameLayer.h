#ifndef GAMELAYER_H
#define GAMELAYER_H

#include "core/engine/Layer.h"

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
};

#endif // GAMELAYER_H
