#ifndef CD_COMPONENTS_RENDERING_COMMANDS_RENDERDEBUGCOMMAND_H
#define CD_COMPONENTS_RENDERING_COMMANDS_RENDERDEBUGCOMMAND_H

#include "IRenderCommand.h"
#include <raylib.h>

// Command to render a bounding box (debug visualization)
class RenderBoundingBoxCommand : public IRenderCommand
{
public:
    RenderBoundingBoxCommand(BoundingBox box, Color color) : m_box(box), m_color(color)
    {
    }

    void Execute() override
    {
        DrawBoundingBox(m_box, m_color);
    }

    const char *GetCommandType() const override
    {
        return "RenderBoundingBox";
    }

private:
    BoundingBox m_box;
    Color m_color;
};

// Command to render a wireframe cube
class RenderCubeWiresCommand : public IRenderCommand
{
public:
    RenderCubeWiresCommand(Vector3 position, float width, float height, float length, Color color)
        : m_position(position), m_width(width), m_height(height), m_length(length), m_color(color)
    {
    }

    void Execute() override
    {
        DrawCubeWires(m_position, m_width, m_height, m_length, m_color);
    }

    const char *GetCommandType() const override
    {
        return "RenderCubeWires";
    }

private:
    Vector3 m_position;
    float m_width;
    float m_height;
    float m_length;
    Color m_color;
};

// Command to render a grid
class RenderGridCommand : public IRenderCommand
{
public:
    RenderGridCommand(int slices, float spacing) : m_slices(slices), m_spacing(spacing)
    {
    }

    void Execute() override
    {
        DrawGrid(m_slices, m_spacing);
    }

    const char *GetCommandType() const override
    {
        return "RenderGrid";
    }

private:
    int m_slices;
    float m_spacing;
};

#endif // CD_COMPONENTS_RENDERING_COMMANDS_RENDERDEBUGCOMMAND_H




