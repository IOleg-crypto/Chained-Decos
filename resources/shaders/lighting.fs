#version 430 core

#include "include/lighting_common.glsl"
#include "include/lighting_directional.glsl"
#include "include/lighting_point.glsl"
#include "include/lighting_spot.glsl"
#include "include/fog.glsl"
#include "include/color_space.glsl"
#include "include/material_pbr.glsl"
#include "include/albedo_sampling.glsl"
#include "include/normal_sampling.glsl"
#include "include/tonemap.glsl"
#include "include/debug_modes.glsl"

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in mat3 fragTBN;

out vec4 finalColor;

void main()
{
    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    vec4 baseColor = SampleAlbedo(fragTexCoord, fragColor, useTexture);
    
    // Alpha discard (Cutout)
    if (ShouldDiscardPixel(baseColor.a, 0.1)) 
        discard;

    int mode = int(uMode + 0.5);
    
    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    if (mode == DEBUG_MODE_UNLIT_ALBEDO) 
    { 
        vec3 unlitLinear = ApplyLinearFog(baseColor.rgb, fragPosition, viewPos, lightDir, lightColor.rgb);
        finalColor = vec4(ToSRGB(unlitLinear), baseColor.a); 
        return; 
    }
    
    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    vec3 normal = CalculateNormal(fragNormal, fragTBN, fragTexCoord, useNormalMap);
    
    
    if (mode == DEBUG_MODE_NORMALS) 
    { 
        finalColor = DebugNormals(normal, 1.0); 
        return; 
    }

    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    float metalness, roughness, occlusion;
    CalculateMaterialProperties(fragTexCoord, metalness, roughness, occlusion);
    
    float shininess;
    vec3 specColor, diffColor;
    CalculateSpecularProperties(metalness, roughness, baseColor.rgb, shininess, specColor, diffColor);
    
    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    if (mode == DEBUG_MODE_LIGHTING_ONLY) 
    {
        baseColor = vec4(0.5, 0.5, 0.5, 1.0);
    }
    else if (mode == DEBUG_MODE_METALNESS || mode == DEBUG_MODE_ROUGHNESS || mode == DEBUG_MODE_OCCLUSION)
    {
        vec4 debugColor = ApplyDebugMode(mode, normal, baseColor.rgb, metalness, roughness, occlusion, baseColor.a);
        finalColor = debugColor;
        return;
    }

    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 skyAmbient = skyAmbientColor.rgb * skyAmbientColor.a; 
    vec3 finalAmbient = diffColor * (ambient + skyAmbient);
    vec3 accumulatedLighting = finalAmbient * occlusion;

    
    accumulatedLighting += CalcDirectionalLight(lightDir, lightColor, normal, viewDir, diffColor, specColor, shininess);

    
    int lightCount = clamp(uLightCount, 0, MAX_LIGHTS);
    for (int i = 0; i < lightCount; i++)
    {
        if (lights[i].enabled == 0) continue;
        
        if (lights[i].type == 0) // Point Light
            accumulatedLighting += CalcPointLight(lights[i], normal, fragPosition, viewDir, diffColor, specColor, shininess);
        else if (lights[i].type == 1) // Spot Light
            accumulatedLighting += CalcSpotLight(lights[i], normal, fragPosition, viewDir, diffColor, specColor, shininess);
    }

    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    vec3 emissiveComp = colEmissive.rgb;
    if (useEmissiveTexture == 1) 
    {
        emissiveComp *= ToLinear(texture(texture5, fragTexCoord).rgb);
    }
    emissiveComp *= emissiveIntensity;

    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    vec3 outLinear = accumulatedLighting + emissiveComp;
    outLinear = ApplyLinearFog(outLinear, fragPosition, viewPos, lightDir, lightColor.rgb);

    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    vec3 outSRGB = FinalizeColor(outLinear, uExposure, 2.2);

    // ════════════════════════════════════════════════════════════════
    
    // ════════════════════════════════════════════════════════════════
    finalColor = vec4(outSRGB, (mode == DEBUG_MODE_LIGHTING_ONLY) ? 1.0 : baseColor.a);
}