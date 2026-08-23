#version 330 core

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float uTime;
uniform float uIntensity = 0.0;
uniform sampler2D texture0;
uniform float uExposure;
uniform float uGamma;

void main()
{
    vec2 uv = fragTexCoord;
    float safeIntensity = clamp(uIntensity, 0.0, 1.0);

    if (safeIntensity < 0.05)
    {
        vec3 color = texture(texture0, uv).rgb;
        color *= uExposure;
        color = pow(color, vec3(1.0 / uGamma));
        finalColor = vec4(color, 1.0);
        return;
    }

    // High-frequency camera shake
    float shakeX = sin(uTime * 45.0) * cos(uTime * 20.0) * sin(uTime * 35.0);
    float shakeY = cos(uTime * 50.0) * sin(uTime * 25.0) * cos(uTime * 40.0);
    
    vec2 offset = vec2(shakeX, shakeY) * safeIntensity * 0.02;

    vec3 c1 = texture(texture0, uv + offset).rgb;
    vec3 c2 = texture(texture0, uv + offset * 0.5).rgb;
    vec3 c3 = texture(texture0, uv).rgb;
    vec3 c4 = texture(texture0, uv - offset * 0.5).rgb;
    vec3 c5 = texture(texture0, uv - offset).rgb;
    
    vec3 blurredShake = (c1 + c2 + c3 + c4 + c5) / 5.0;

    blurredShake *= uExposure;
    blurredShake = pow(blurredShake, vec3(1.0 / uGamma));

    finalColor = vec4(blurredShake, 1.0);
}
