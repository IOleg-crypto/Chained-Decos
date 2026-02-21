#version 430 core

#include "include/lighting_common.glsl"
#include "include/lighting_directional.glsl"
#include "include/lighting_point.glsl"
#include "include/lighting_spot.glsl"
#include "include/fog.glsl"

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in mat3 fragTBN;

out vec4 finalColor;

void main()
{
    // 1. Base Color & Diagnostics
    // 1. Base Color & Diagnostics
    vec4 baseColor = colDiffuse;
    
    // Fix: If fragColor (Vertex Color) is close to black (default attribute 0,0,0,1), ignore it.
    // Otherwise multiply. This prevents generated meshes (no colors) from being black.
    if (length(fragColor.rgb) > 0.01) baseColor *= fragColor;
    
    if (useTexture == 1) baseColor *= texture(texture0, fragTexCoord);
    
    int mode = int(uMode + 0.5);
    if (mode == 3) { finalColor = baseColor; return; }
    
    // Normal Mapping
    vec3 normal = normalize(fragNormal);
    if (useNormalMap == 1) {
        vec3 mapNormal = texture(texture2, fragTexCoord).rgb;
        mapNormal = normalize(mapNormal * 2.0 - 1.0);
        normal = normalize(fragTBN * mapNormal);
    }
    
    if (mode == 1) { finalColor = vec4(normal * 0.5 + 0.5, 1.0); return; }

    // PBR Properties
    float m = metalness;
    float r = roughness;
    float occ = 1.0;

    if (useMetallicMap == 1) m *= texture(texture1, fragTexCoord).b; // Blue channel usually
    if (useRoughnessMap == 1) r *= texture(texture3, fragTexCoord).g; // Green channel usually
    if (useOcclusionMap == 1) occ = texture(texture4, fragTexCoord).r;

    // Map PBR to Blinn-Phong (Approximation)
    // Roughness -> Shininess
    float s = (1.0 - r) * 128.0;
    if (s < 1.0) s = 1.0;
    
    // Metalness -> Specular Color & Diffuse
    // Dielectric: Specular is white-ish (low), Diffuse is Albedo
    // Metal: Specular is Albedo, Diffuse is Black
    vec3 specColor = mix(vec3(0.04), baseColor.rgb, m);
    vec3 diffColor = baseColor.rgb * (1.0 - m);

    // 2. Main Lighting Loop
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 lighting = diffColor * ambient * occ; // Ambient

    
    // Directional Light
    lighting += CalcDirectionalLight(lightDir, lightColor, normal, viewDir, diffColor, s);

    // Dynamic Lights (Unified)
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 0) continue;
        
        if (lights[i].type == 0) // Point
            lighting += CalcPointLight(lights[i], normal, fragPosition, viewDir, diffColor, s);
        else if (lights[i].type == 1) // Spot
            lighting += CalcSpotLight(lights[i], normal, fragPosition, viewDir, diffColor, s);
    }

    // 3. Emissive Component
    vec3 emissiveComp = colEmissive.rgb;
    if (useEmissiveTexture == 1) emissiveComp *= texture(texture5, fragTexCoord).rgb;
    emissiveComp *= emissiveIntensity;

    // 4. Final Assembly
    vec4 result = vec4(lighting + emissiveComp, (mode == 2) ? 1.0 : baseColor.a);
    
    // 5. Apply Fog
    finalColor = ApplyFog(result, fragPosition, viewPos, uTime);
}
