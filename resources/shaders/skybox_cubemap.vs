#version 450 core

layout (location = 0) in vec3 a_Position;

out vec3 v_Position;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    v_Position = a_Position;
    // Remove translation from view matrix
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(a_Position, 1.0);
    gl_Position = pos.xyww;
}
