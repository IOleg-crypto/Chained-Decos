#version 430 core

#include "include/lighting_common.glsl"
#include "include/fog.glsl"

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 u_Tint;
uniform vec2 u_Flip; // x = flipX, y = flipY (1.0 = flip, 0.0 = normal)

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    if (u_Flip.x > 0.5) uv.x = 1.0 - uv.x;
    if (u_Flip.y > 0.5) uv.y = 1.0 - uv.y;

    vec4 texColor = texture(texture0, uv);
    vec4 result = texColor * fragColor * u_Tint;

    if (result.a < 0.01)
        discard;

    finalColor = ApplyFog(result, fragPosition, viewPos, uTime);
}
