#version 430 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec3 fragPosition;

void main()
{
    fragTexCoord = a_TexCoord;
    fragPosition = a_Position;
    gl_Position = mvp * vec4(a_Position, 1.0);
}
