#version 430

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// Light data
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform float ambient;

#define MAX_LIGHTS 8

struct Light {
    vec3 position;
    vec4 color;
    float radius;
    float radiance;
    float falloff;
    int enabled;
};

uniform Light lights[MAX_LIGHTS];

// Fog data
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

uniform vec3 viewPos;
uniform float uTime;

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

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 diffuse = colDiffuse * fragColor;

    // Calculate ambient light
    vec4 ambientColor = diffuse * ambient;

    // Calculate diffuse light (directional)
    float diff = max(dot(fragNormal, normalize(-lightDir)), 0.0);
    vec4 diffuseColor = diffuse * lightColor * diff;

    // Point lights
    vec4 pointLightsColor = vec4(0.0);
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 0) continue;

        vec3 lightVector = lights[i].position - fragPosition;
        float distance = length(lightVector);
        
        if (distance < lights[i].radius)
        {
            vec3 L = normalize(lightVector);
            float NdotL = max(dot(fragNormal, L), 0.0);
            
            // Basic falloff
            float attenuation = pow(clamp(1.0 - distance/lights[i].radius, 0.0, 1.0), lights[i].falloff);
            
            pointLightsColor += diffuse * lights[i].color * NdotL * attenuation * lights[i].radiance;
        }
    }

    // Final color before fog
    vec4 finalColorPreFog = (ambientColor + diffuseColor + pointLightsColor) * texelColor;

    if (fogEnabled == 1)
    {
        // Distance-based fog from camera position
        float dist = length(viewPos - fragPosition);
        
        // Multi-frequency noise for volumetric feel
        vec3 noisePos = fragPosition * 0.05 + vec3(uTime * 0.1, 0.0, uTime * 0.05);
        float volumetricFactor = 0.5 + noise(noisePos) * 1.0;
        
        // 1. Linear fog factor
        float linearFog = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
        
        // 2. Exponential fog factor (modulated by noise)
        float expFog = exp(-dist * fogDensity * volumetricFactor);
        expFog = clamp(expFog, 0.0, 1.0);
        
        // Combine: we take the most "foggy" result
        float fogFactor = min(linearFog, expFog);
        
        finalColor = mix(fogColor, finalColorPreFog, fogFactor);
        finalColor.a = mix(fogColor.a, finalColorPreFog.a, fogFactor);
    }
    else
    {
        finalColor = finalColorPreFog;
    }
}
