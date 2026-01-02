#ifndef CD_COMPONENTS_RENDERING_COMMANDS_RENDERPRIMITIVECOMMAND_H
#define CD_COMPONENTS_RENDERING_COMMANDS_RENDERPRIMITIVECOMMAND_H

#include "IRenderCommand.h"
#include <raylib.h>

// Command to render a cube
class RenderCubeCommand : public IRenderCommand
{
public:
    RenderCubeCommand(Vector3 position, float width, float height, float length, Color color)
        : m_position(position), m_width(width), m_height(height), m_length(length), m_color(color)
    {
    }

    void Execute() override
    {
        DrawCube(m_position, m_width, m_height, m_length, m_color);
    }

    const char *GetCommandType() const override
    {
        return "RenderCube";
    }

private:
    Vector3 m_position;
    float m_width;
    float m_height;
    float m_length;
    Color m_color;
};

// Command to render a sphere
class RenderSphereCommand : public IRenderCommand
{
public:
    RenderSphereCommand(Vector3 centerPos, float radius, Color color)
        : m_centerPos(centerPos), m_radius(radius), m_color(color)
    {
    }

    void Execute() override
    {
        DrawSphere(m_centerPos, m_radius, m_color);
    }

    const char *GetCommandType() const override
    {
        return "RenderSphere";
    }

private:
    Vector3 m_centerPos;
    float m_radius;
    Color m_color;
};

// Command to render a plane
class RenderPlaneCommand : public IRenderCommand
{
public:
    RenderPlaneCommand(Vector3 centerPos, Vector2 size, Color color)
        : m_centerPos(centerPos), m_size(size), m_color(color)
    {
    }

    void Execute() override
    {
        DrawPlane(m_centerPos, m_size, m_color);
    }

    const char *GetCommandType() const override
    {
        return "RenderPlane";
    }

private:
    Vector3 m_centerPos;
    Vector2 m_size;
    Color m_color;
};

#endif // CD_COMPONENTS_RENDERING_COMMANDS_RENDERPRIMITIVECOMMAND_H




