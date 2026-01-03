#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/layer.h"
#include <raylib.h>

namespace CH
{
class EditorLayer : public Layer
{
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender() override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event &e) override;

private:
    Camera3D m_EditorCamera;
};
} // namespace CH

#endif // CH_EDITOR_LAYER_H
