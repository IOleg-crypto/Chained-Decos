#version 450 core

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec3 fragPosition;

// Input uniform values
layout(binding = 0) uniform sampler2D environmentMap;
uniform bool doGamma;
uniform float fragGamma;
uniform float exposure;
uniform float brightness;
uniform float contrast;

// Output fragment color
layout(location = 0) out vec4 finalColor;

#define PI 3.14159265358979323846

void main()
{
    vec3 v = normalize(fragPosition);
    
    // Convert Cartesian direction to equirectangular UVs
    vec2 uv = vec2(atan(v.z, v.x) / (2.0 * PI) + 0.5, asin(v.y) / PI + 0.5);
    
    // Fetch color from texture map
    vec3 color = texture(environmentMap, uv).rgb;

    // Apply exposure
    color *= exposure;

    // Apply brightness
    color += brightness;

    // Apply contrast
    color = (color - 0.5) * contrast + 0.5;

    if (doGamma) // Apply gamma correction
    {
        color = pow(color, vec3(1.0/fragGamma));
    }

    // Calculate final fragment color
    finalColor = vec4(color, 1.0);
}
