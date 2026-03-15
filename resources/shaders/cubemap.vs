#version 450 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 matProjection;
uniform mat4 matView;

layout(location = 0) out vec3 fragPosition;

void main()
{
    fragPosition = vertexPosition;
    gl_Position = matProjection * matView * vec4(vertexPosition, 1.0);
}
