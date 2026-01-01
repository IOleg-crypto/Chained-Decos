#version 450 core

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec3 fragPosition;

// Input uniform values
layout(binding = 0) uniform samplerCube environmentMap;
uniform bool vflipped;
uniform bool doGamma;
uniform float fragGamma;
uniform float exposure;
uniform float brightness;
uniform float contrast;

// Output fragment color
layout(location = 0) out vec4 finalColor;

void main()
{
    // Fetch color from texture map
    vec3 color = vec3(0.0);

    if (vflipped)
    {
        color = texture(environmentMap, vec3(fragPosition.x, -fragPosition.y, fragPosition.z)).rgb;
    }
    else
    {
        color = texture(environmentMap, fragPosition).rgb;
    }

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