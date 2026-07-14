#version 330 core

layout (location = 0) in vec3 a_Position;

#include "../include/camera.glsl"
uniform mat4 u_Model;

out vec3 v_WorldPos;

void main()
{
    v_WorldPos = (u_Model * vec4(a_Position, 1.0)).xyz;
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}
