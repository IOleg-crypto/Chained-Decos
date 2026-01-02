#ifndef CD_COMPONENTS_RENDERING_COMMANDS_RENDERMODELCOMMAND_H
#define CD_COMPONENTS_RENDERING_COMMANDS_RENDERMODELCOMMAND_H

#include "IRenderCommand.h"
#include <raylib.h>
#include <string>

// Command to render a 3D model
class RenderModelCommand : public IRenderCommand
{
public:
    RenderModelCommand(Model model, Vector3 position, float scale, Color tint = WHITE)
        : m_model(model), m_position(position), m_scale(scale), m_tint(tint)
    {
    }

    void Execute() override
    {
        DrawModel(m_model, m_position, m_scale, m_tint);
    }

    const char *GetCommandType() const override
    {
        return "RenderModel";
    }

private:
    Model m_model;
    Vector3 m_position;
    float m_scale;
    Color m_tint;
};

// Command to render a model with custom transform
class RenderModelTransformCommand : public IRenderCommand
{
public:
    RenderModelTransformCommand(Model model, Matrix transform, Color tint = WHITE)
        : m_model(model), m_transform(transform), m_tint(tint)
    {
    }

    void Execute() override
    {
        DrawModel(m_model, {0, 0, 0}, 1.0f, m_tint);
    }

    const char *GetCommandType() const override
    {
        return "RenderModelTransform";
    }

private:
    Model m_model;
    Matrix m_transform;
    Color m_tint;
};

#endif // CD_COMPONENTS_RENDERING_COMMANDS_RENDERMODELCOMMAND_H




