#version 430 core

#include "../include/surface.glsl"
#include "../include/lighting.glsl"
#include "../include/shadow.glsl"
#include "../include/fog.glsl"
#include "../include/debug_modes.glsl"

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in mat3 fragTBN;
in vec4 fragPosLightSpace;

out vec4 finalColor;

void main()
{
    // 1. Gather all surface and material properties
    Surface surf = CreateSurface(fragPosition, fragNormal, fragTBN, fragTexCoord, fragColor, viewPos);

    // Alpha test discard
    if (surf.alpha < 0.1)
        discard;

    int mode = int(uMode + 0.5);

    // 2. Diagnostics / View modes
    if (mode == DEBUG_MODE_NORMALS)
    {
        finalColor = vec4(surf.normal * 0.5 + 0.5, 1.0);
        return;
    }
    else if (mode == DEBUG_MODE_UNLIT_ALBEDO)
    {
        vec3 unlitLinear = ApplyLinearFog(surf.albedo, surf.position, viewPos, lightDir, lightColor.rgb);
        finalColor = vec4(unlitLinear, surf.alpha);
        return;
    }
    else if (mode == DEBUG_MODE_METALNESS)
    {
        finalColor = vec4(vec3(surf.metalness), surf.alpha);
        return;
    }
    else if (mode == DEBUG_MODE_ROUGHNESS)
    {
        finalColor = vec4(vec3(surf.roughness), surf.alpha);
        return;
    }
    else if (mode == DEBUG_MODE_OCCLUSION)
    {
        finalColor = vec4(vec3(surf.occlusion), surf.alpha);
        return;
    }
    else if (mode == DEBUG_MODE_LIGHTING_ONLY)
    {
        surf.albedo = vec3(0.5);
        surf.diffuse = vec3(0.5);
        surf.specular = vec3(0.04);
        surf.shininess = 32.0;
    }

    // 3. Ambient & Sky Environment Lighting
    vec3 accumulatedLighting = CalcAmbientLighting(surf, skyAmbientColor, ambient);

    // 4. Main Directional Light (Sun, Moon, etc.) + Shadows
    float shadow = ShadowCalculationPCF(fragPosLightSpace);
    vec3 mainLightContrib = CalcDirectionalLight(lightDir, lightColor, 1.0, surf);
    accumulatedLighting += mainLightContrib * shadow;

    // 5. Dynamic Lights (Point, Spot, Secondary Directional from SSBO)
    int lightCount = clamp(uLightCount, 0, MAX_LIGHTS);
    for (int i = 0; i < lightCount; i++)
    {
        if (lights[i].enabled == 0) continue;
        accumulatedLighting += CalcDynamicLight(lights[i], surf);
    }

    // 6. Emissive + Fog
    vec3 outLinear = accumulatedLighting + surf.emissive;
    outLinear = ApplyLinearFog(outLinear, surf.position, viewPos, lightDir, lightColor.rgb);

    finalColor = vec4(outLinear, (mode == DEBUG_MODE_LIGHTING_ONLY) ? 1.0 : surf.alpha);
}
