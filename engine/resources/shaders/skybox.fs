#version 450 core

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec3 fragPosition;

// Input uniform values
layout(binding = 0) uniform sampler2D texture0;
uniform bool vflipped;
uniform bool doGamma;
uniform float fragGamma;
uniform float exposure;
uniform float brightness;
uniform float contrast;
uniform float uTime;

// Fog data
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

// Output fragment color
layout(location = 0) out vec4 finalColor;

uniform int skyboxMode; // 0: Equirectangular, 1: Cross (Horizontal)

float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(p + vec3(0, 0, 0)), hash(p + vec3(1, 0, 0)), f.x),
                   mix(hash(p + vec3(0, 1, 0)), hash(p + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(hash(p + vec3(0, 0, 1)), hash(p + vec3(1, 0, 1)), f.x),
                   mix(hash(p + vec3(0, 1, 1)), hash(p + vec3(1, 1, 1)), f.x), f.y), f.z);
}

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

// Convert 3D direction to UV for a Horizontal Cross layout
// Layout assumption: 4 columns, 3 rows.
//      [+Y]
// [-X] [+Z] [+X] [-Z]
//      [-Y]
vec2 SampleCubeMap(vec3 v)
{
    vec3 absV = abs(v);
    float maxV = max(max(absV.x, absV.y), absV.z);
    
    vec2 uv = vec2(0.0);
    vec2 offset = vec2(0.0);
    
    // Normalize to face
    vec3 n = v / maxV;
    
    if (absV.x >= absV.y && absV.x >= absV.z) {
        // X Plane
        if (v.x > 0.0) { // +X (Right) -> Col 2, Row 1 (0-indexed: Col 2)
            uv = vec2(-n.z, n.y); // Rotate? Check standard
            offset = vec2(2.0, 1.0);
        } else { // -X (Left) -> Col 0, Row 1
            uv = vec2(n.z, n.y);
            offset = vec2(0.0, 1.0);
        }
    } else if (absV.y >= absV.x && absV.y >= absV.z) {
        // Y Plane
        if (v.y > 0.0) { // +Y (Top) -> Col 1, Row 0
            uv = vec2(n.x, -n.z);
            offset = vec2(1.0, 0.0);
        } else { // -Y (Bottom) -> Col 1, Row 2
            uv = vec2(n.x, n.z);
            offset = vec2(1.0, 2.0);
        }
    } else {
        // Z Plane
        if (v.z > 0.0) { // +Z (Front) -> Col 1, Row 1
            uv = vec2(n.x, n.y);
            offset = vec2(1.0, 1.0);
        } else { // -Z (Back) -> Col 3, Row 1
            uv = vec2(-n.x, n.y);
            offset = vec2(3.0, 1.0);
        }
    }
    
    // Transform from [-1, 1] to [0, 1]
    uv = (uv + 1.0) * 0.5;
    
    // Apply offset and scale (1/4 width, 1/3 height)
    uv.x = (uv.x + offset.x) / 4.0;
    uv.y = (uv.y + offset.y) / 3.0; // V might need flip depending on texture origin
    
    return uv;
}

void main()
{
    vec3 dir = normalize(fragPosition);
    
    vec2 uv;
    if (skyboxMode == 1) uv = SampleCubeMap(dir);
    else uv = SampleSphericalMap(dir);
    
    if (vflipped) uv.y = 1.0 - uv.y;

    vec3 color = texture(texture0, uv).rgb;

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

    vec4 background = vec4(color, 1.0);

    if (fogEnabled == 1)
    {
        // Horizon fog: thicker near the horizon line
        float horizonEffect = 1.0 - abs(dir.y);
        horizonEffect = pow(horizonEffect, 3.0);
        
        // Volumetric noise for "mist patches" in the distance
        vec3 noiseDir = dir * 2.0 + vec3(uTime * 0.05, uTime * 0.02, uTime * 0.03);
        float n = noise(noiseDir);
        
        // Combine horizon and noise, modulated by fog density
        float fogFactor = clamp(horizonEffect + n * 0.4, 0.0, 1.0);
        
        // Apply global density boost
        fogFactor = mix(fogFactor, 1.0, clamp(fogDensity * 10.0, 0.0, 1.0));
        
        finalColor = mix(background, fogColor, fogFactor);
    }
    else
    {
        finalColor = background;
    }
}
