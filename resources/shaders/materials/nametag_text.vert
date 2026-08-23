#version 430 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    TexCoords = a_TexCoord;
    gl_Position = projection * vec4(a_Position, 0.0, 1.0);
}
