#version 450 core

layout(location = 0) in vec3 fragPosition;

uniform sampler2D texture0;

uniform int vflipped;
uniform int isHDR;
uniform float exposure;
uniform float brightness;
uniform float contrast;
uniform int skyboxMode; // 0: Sphere, 1: Direct/Cross

in vec2 fragTexCoord; // Need UVs for Mode 1

// Fog uniforms
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;

layout(location = 0) out vec4 finalColor;

// Converts 3D direction to Spherical UVs for equirectangular panorama
vec2 SampleSpherical(vec3 dir)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    return uv + 0.5;
}

// Note: ACES Tonemapping and Gamma correction are handled globally in post_process.fs

void main()
{
    vec3 direction = normalize(fragPosition);
    vec2 uv;
    
    if (skyboxMode == 1) {
        // Mode 1: Direct Mapping (uses texture coordinates from the mesh)
        uv = fragTexCoord;
    } else {
        // Mode 0: Spherical Mapping (Equirectangular)
        uv = SampleSpherical(direction);
    }

    if (vflipped == 1) uv.y = 1.0 - uv.y;

    // Sample the panorama
    vec3 color = texture(texture0, uv).rgb;

    // 1. Exposure & Color corrections
    color *= exposure;
    color += brightness;
    color = (color - 0.5) * contrast + 0.5;

    vec4 background = vec4(color, 1.0);

    // 2. Unified Horizon & Ground Fog
    // This ensures that anything below the horizon eventually matches the fog color
    // preventing "holes" or sharp seams.
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