#version 430 core

layout(location = 0) in vec2 in_quadVertices;
layout(location = 1) in vec2 in_texCoords;

out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 modelPosition;
uniform vec2 size;

void main()
{
    TexCoords = in_texCoords;

    // Translate player position into view space
    vec4 viewPos = view * vec4(modelPosition, 1.0);

    // Offset quad vertices directly in view space (always faces camera)
    viewPos.xy += in_quadVertices * size;

    // Project to screen
    gl_Position = projection * viewPos;
}
