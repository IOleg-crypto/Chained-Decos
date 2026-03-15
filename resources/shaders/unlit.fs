#version 430

#include "include/lighting_common.glsl"
#include "include/fog.glsl"

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

out vec4 finalColor;

void main()
{
    vec4 baseColor = colDiffuse * fragColor;
    if (useTexture == 1) baseColor *= texture(texture0, fragTexCoord);
    
    vec3 emissiveComp = colEmissive.rgb;
    if (useEmissiveTexture == 1) emissiveComp *= texture(texture1, fragTexCoord).rgb;
    emissiveComp *= emissiveIntensity;

    vec4 result = vec4(baseColor.rgb + emissiveComp, baseColor.a);
    finalColor = ApplyFog(result, fragPosition, viewPos, uTime);
}
