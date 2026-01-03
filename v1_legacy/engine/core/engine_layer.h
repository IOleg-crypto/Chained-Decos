#ifndef CD_ENGINE_CORE_ENGINE_LAYER_H
#define CD_ENGINE_CORE_ENGINE_LAYER_H

#include "engine/core/layer/layer.h"

namespace CHEngine
{
/**
 * @brief The EngineLayer is responsible for the simulation lifecycle.
 *
 * It initializes high-level systems like Physics, Audio, and Scene management,
 * which were previously (incorrectly) part of the Core Application.
 */
class EngineLayer : public Layer
{
public:
    EngineLayer();
    virtual ~EngineLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnEvent(Event &event) override;
};
} // namespace CHEngine

#endif // CD_ENGINE_CORE_ENGINE_LAYER_H
