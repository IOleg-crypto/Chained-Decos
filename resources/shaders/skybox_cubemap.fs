#version 450 core

layout(location = 0) in vec3 fragPosition;

uniform samplerCube environmentMap;

uniform int isHDR;
uniform float exposure;
uniform float brightness;
uniform float contrast;

// Fog uniforms
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;

layout(location = 0) out vec4 finalColor;

// Note: Tonemapping and Gamma correction are handled globally in post_process.fs

void main()
{
    vec3 direction = normalize(fragPosition);
    
    // Sample the environment cubemap
    vec3 color = texture(environmentMap, direction).rgb;

    // 1. Exposure & Color Corrections
    color *= exposure;
    color += brightness;
    color = (color - 0.5) * contrast + 0.5;

    vec4 background = vec4(color, 1.0);

    // 2. Unified Horizon & Ground Fog
    if (fogEnabled == 1) {
        // Simple vertical gradient: 1.0 at nadir (-Y), 0.0 at top (+Y)
        float verticalFactor = clamp(1.0 - (direction.y + 0.05) * 10.0, 0.0, 1.0);
        float fogFactor = pow(verticalFactor, 2.0); // Smooth curve for horizon focus
        
        // Also add a slight horizon "haze" even looking up
        float horizonHaze = pow(1.0 - abs(direction.y), 5.0) * 0.5;
        fogFactor = max(fogFactor, horizonHaze);
        
        fogFactor = clamp(fogFactor * clamp(fogDensity * 5.0, 0.0, 1.0), 0.0, 1.0);
        finalColor = mix(background, fogColor, fogFactor);
    } else {
        finalColor = background;
    }
}
