#version 430 core

#include "include/lighting_common.glsl"
#include "include/lighting_directional.glsl"
#include "include/lighting_point.glsl"
#include "include/lighting_spot.glsl"
#include "include/fog.glsl"
#include "include/color_space.glsl"

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in mat3 fragTBN;

out vec4 finalColor;

void main()
{
    // 1. Base color
    vec4 baseColor = colDiffuse;
    
    // Vertex color contribution (if non-zero)
    if (length(fragColor.rgb) > 0.01) 
    {
        baseColor *= fragColor;
    }
    
    // Albedo texture (in sRGB space from file — convert to linear)
    if (useTexture == 1) 
    {
        vec4 sampled = texture(texture0, fragTexCoord);
        baseColor.rgb *= ToLinear(sampled.rgb); 
        baseColor.a   *= sampled.a;
    }
    
    // Alpha discard (Cutout)
    if (baseColor.a < 0.1) discard;
    
    int mode = int(uMode + 0.5);
    if (mode == 2) baseColor = vec4(0.5, 0.5, 0.5, 1.0); // Neutral grey for Lighting-only
    
    // Albedo/Unlit mode — apply fog and exit before lighting
    if (mode == 3) 
    { 
        finalColor = ApplyFog(baseColor, fragPosition, viewPos, uTime); 
        return; 
    }
    
    // Normal calculation
    vec3 normal = normalize(fragNormal);
    if (useNormalMap == 1) {
        vec3 mapNormal = texture(texture2, fragTexCoord).rgb;
        mapNormal = normalize(mapNormal * 2.0 - 1.0);
        normal = normalize(fragTBN * mapNormal);
    }
    
    if (mode == 1) { finalColor = vec4(normal * 0.5 + 0.5, 1.0); return; }

    // PBR parameters (Blinn-Phong approximation)
    float m = metalness;
    float r = roughness;
    float occ = 1.0;

    if (useMetallicMap == 1) m *= texture(texture1, fragTexCoord).b; 
    if (useRoughnessMap == 1) r *= texture(texture3, fragTexCoord).g; 
    if (useOcclusionMap == 1) occ = texture(texture4, fragTexCoord).r;

    m = clamp(m, 0.0, 1.0);
    r = clamp(r, 0.04, 1.0);

    float s = (1.0 - r) * 128.0;
    if (s < 1.0) s = 1.0;
    
    vec3 specColor = mix(vec3(0.04), baseColor.rgb, m);
    vec3 diffColor = baseColor.rgb * (1.0 - m);

    // Ambient
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 skyAmbient = skyAmbientColor.rgb * skyAmbientColor.a; 
    vec3 finalAmbient = diffColor * (ambient + skyAmbient);
    vec3 lighting = finalAmbient * occ;

    // Directional light
    lighting += CalcDirectionalLight(lightDir, lightColor, normal, viewDir, diffColor, specColor, s);

    // Dynamic lights from SSBO
    int lightCount = clamp(uLightCount, 0, MAX_LIGHTS);
    for (int i = 0; i < lightCount; i++)
    {
        if (lights[i].enabled == 0) continue;
        
        if (lights[i].type == 0) // Point Light
            lighting += CalcPointLight(lights[i], normal, fragPosition, viewDir, diffColor, specColor, s);
        else if (lights[i].type == 1) // Spot Light
            lighting += CalcSpotLight(lights[i], normal, fragPosition, viewDir, diffColor, specColor, s);
    }

    // Emissive (already stored in linear — direct multiplication is correct)
    vec3 emissiveComp = colEmissive.rgb;
    if (useEmissiveTexture == 1) 
    {
        emissiveComp *= ToLinear(texture(texture5, fragTexCoord).rgb);
    }
    emissiveComp *= emissiveIntensity;

    // Combine in linear space
    vec3 outLinear = lighting + emissiveComp;

    // Exposure tone-mapping in linear space
    outLinear = vec3(1.0) - exp(-outLinear * uExposure);

    // Gamma correction (linear → sRGB)
    vec3 outSRGB = ToSRGB(outLinear);

    vec4 result = vec4(outSRGB, (mode == 2) ? 1.0 : baseColor.a);

    // Apply world fog
    finalColor = ApplyFog(result, fragPosition, viewPos, uTime);
}