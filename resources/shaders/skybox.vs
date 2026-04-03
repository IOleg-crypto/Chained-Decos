#version 450 core

layout(location = 0) in vec3 a_Position;

out vec3 v_Position;

uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    v_Position = a_Position;
    // Position is already normalized since it's a unit sphere/cube
    gl_Position = u_Projection * u_View * vec4(a_Position, 1.0);
    gl_Position = gl_Position.xyww;
}
