#version 450 core

// Infinite Grid Vertex Shader

layout(location = 0) in vec3 vertexPosition;

layout(location = 0) uniform mat4 matView;
layout(location = 1) uniform mat4 matProjection;

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;
layout(location = 2) out mat4 fragView;
layout(location = 6) out mat4 fragProj;

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec3 p = vertexPosition;
    nearPoint = UnprojectPoint(p.x, p.y, -1.0, matView, matProjection);
    farPoint = UnprojectPoint(p.x, p.y, 1.0, matView, matProjection);
    fragView = matView;
    fragProj = matProjection;
    gl_Position = vec4(p, 1.0);
}
