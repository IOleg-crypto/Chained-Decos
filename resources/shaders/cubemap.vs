#version 450 core

layout (location = 0) in vec3 a_Position;

uniform mat4 projection;
uniform mat4 view;

out vec3 fragPosition;

void main()
{
    fragPosition = a_Position;
    
    // Remove translation from view matrix to keep skybox centered on camera
    mat4 viewRotationOnly = mat4(mat3(view));
    
    vec4 pos = projection * viewRotationOnly * vec4(a_Position, 1.0);
    
    // We don't use .xyww here because we raw into an empty FBO without depth buffer.
    // Otherwise Z=1.0 might be clipped.
    gl_Position = pos;
}