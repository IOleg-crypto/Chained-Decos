#version 430

#include "include/lighting_common.glsl"
#include "include/lighting_directional.glsl"
#include "include/lighting_point.glsl"
#include "include/lighting_spot.glsl"
#include "include/fog.glsl"

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

out vec4 finalColor;

void main()
{
    // 1. Base Color & Diagnostics
    vec4 baseColor = colDiffuse * fragColor;
    if (useTexture == 1) baseColor *= texture(texture0, fragTexCoord);
    
    int mode = int(uMode + 0.5);
    if (mode == 3) { finalColor = baseColor; return; }
    
    vec3 normal = normalize(fragNormal);
    if (mode == 1) { finalColor = vec4(normal * 0.5 + 0.5, 1.0); return; }

    // 2. Main Lighting Loop
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 lighting = baseColor.rgb * ambient; // Ambient
    
    // Directional Light
    lighting += CalcDirectionalLight(lightDir, lightColor, normal, viewDir, baseColor.rgb, shininess);

    // Point Lights
    for (int i = 0; i < MAX_LIGHTS; i++)
        lighting += CalcPointLight(pointLights[i], normal, fragPosition, viewDir, baseColor.rgb, shininess);

    // Spot Lights
    for (int i = 0; i < MAX_LIGHTS; i++)
        lighting += CalcSpotLight(spotLights[i], normal, fragPosition, viewDir, baseColor.rgb, shininess);

    // 3. Emissive Component
    vec3 emissiveComp = colEmissive.rgb;
    if (useEmissiveTexture == 1) emissiveComp *= texture(texture1, fragTexCoord).rgb;
    emissiveComp *= emissiveIntensity;

    // 4. Final Assembly
    vec4 result = vec4(lighting + emissiveComp, (mode == 2) ? 1.0 : baseColor.a);
    
    // 5. Apply Fog
    finalColor = ApplyFog(result, fragPosition, viewPos, uTime);
}
