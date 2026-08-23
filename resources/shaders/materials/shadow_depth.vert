#version 430 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 matModel;

void main()
{
    gl_Position = u_LightSpaceMatrix * matModel * vec4(a_Position, 1.0);
}
