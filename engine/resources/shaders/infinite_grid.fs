#version 450 core

// Infinite Grid Shader (Hazel-style)
// Based on: http://asliceofrendering.com/webgl/2019/06/19/InfiniteGrid/

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;
layout(location = 2) in mat4 fragView;
layout(location = 6) in mat4 fragProj;

layout(location = 0) out vec4 outColor;

vec4 grid(vec3 fragPos3D, float scale, bool drawLines) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);
    
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0 - min(line, 1.0));

    // Z axis (Blue)
    if (fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    // X axis (Red)
    if (fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;
        
    return color;
}

uniform float near;
uniform float far;

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    // In OpenGL, clip space z/w is [-1, 1]. gl_FragDepth expects [0, 1].
    return ((clip_space_pos.z / clip_space_pos.w) + 1.0) * 0.5;
}

float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w); // NDC depth [-1, 1]
    float linearDepth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near));
    return linearDepth / far;
}

void main() {
    float dy = farPoint.y - nearPoint.y;
    // Check if ray is roughly parallel to the plane or doesn't intersect
    if (abs(dy) < 0.0001) discard;

    float t = -nearPoint.y / dy;
    if (t <= 0.0) discard;

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0.0, (1.0 - linearDepth));

    outColor = grid(fragPos3D, 1.0, true);
    outColor.a *= fading;
    
    if (outColor.a < 0.01) discard;
}
