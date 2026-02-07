#version 450 core

// Input vertex attributes
layout(location = 0) in vec3 vertexPosition;

// Input uniform values
layout(location = 0) uniform mat4 matProjection;
layout(location = 1) uniform mat4 matView;

// Output vertex attributes (to fragment shader)
layout(location = 0) out vec3 fragPosition;


void main()
{
    // Calculate fragment position based on model transformations
    fragPosition = vertexPosition;
    // Remove translation from the view matrix
    mat4 rotView = mat4(mat3(matView));
    vec4 clipPos = matProjection*rotView*vec4(vertexPosition, 1.0);

    // Calculate final vertex position
    gl_Position = clipPos.xyww;
}
