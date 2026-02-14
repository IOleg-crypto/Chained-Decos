// Fog data
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

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

vec4 ApplyFog(vec4 color, vec3 fragPos, vec3 viewPos, float time)
{
    if (fogEnabled == 0) return color;

    float dist = length(viewPos - fragPos);
    vec3 noisePos = fragPos * 0.05 + vec3(time * 0.1, 0.0, time * 0.05);
    float volumetricFactor = 0.5 + noise(noisePos) * 1.0;
    
    float linearFog = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
    float expFog = exp(-dist * fogDensity * volumetricFactor);
    float fogFactor = clamp(min(linearFog, expFog), 0.0, 1.0);
    
    vec4 result;
    result.rgb = mix(fogColor.rgb, color.rgb, fogFactor);
    result.a = mix(fogColor.a, color.a, fogFactor);
    return result;
}
