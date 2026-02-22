#version 450 core

layout(location = 0) in vec3 fragPosition;

layout(binding = 0) uniform sampler2D texture0;

uniform int vflipped;
uniform int doGamma;
uniform float fragGamma;
uniform float exposure;
uniform float brightness;
uniform float contrast;
uniform float uTime;
uniform int isHDR;

uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;

uniform int skyboxMode; // 0 = Equirect, 1 = Cross

layout(location = 0) out vec4 finalColor;

////////////////////////////////////////////////////////////
// Tone mapping (ACES)
////////////////////////////////////////////////////////////
vec3 ACES(vec3 x)
{
    return clamp(
        (x * (2.51 * x + 0.03)) /
        (x * (2.43 * x + 0.59) + 0.14),
        0.0, 1.0
    );
}

////////////////////////////////////////////////////////////
// Simple noise
////////////////////////////////////////////////////////////
float hash(vec3 p)
{
    return fract(sin(dot(p, vec3(12.9898,78.233,45.164))) * 43758.5453);
}

////////////////////////////////////////////////////////////
// Equirectangular mapping
////////////////////////////////////////////////////////////
vec2 SampleSpherical(vec3 dir)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    return uv + 0.5;
}

////////////////////////////////////////////////////////////
// Horizontal Cross mapping (4x3)
////////////////////////////////////////////////////////////
vec2 SampleCross(vec3 dir)
{
    vec3 a = abs(dir);
    vec3 n = dir / max(max(a.x, a.y), a.z);

    vec2 uv;
    vec2 offset;

    if (a.x > a.y && a.x > a.z)
    {
        uv = vec2(dir.x > 0.0 ? -n.z : n.z, n.y);
        offset = vec2(dir.x > 0.0 ? 2.0 : 0.0, 1.0);
    }
    else if (a.y > a.x && a.y > a.z)
    {
        uv = vec2(n.x, dir.y > 0.0 ? -n.z : n.z);
        offset = vec2(1.0, dir.y > 0.0 ? 0.0 : 2.0);
    }
    else
    {
        uv = vec2(dir.z > 0.0 ? n.x : -n.x, n.y);
        offset = vec2(dir.z > 0.0 ? 1.0 : 3.0, 1.0);
    }

    uv = (uv + 1.0) * 0.5;
    uv = (uv + offset) / vec2(4.0, 3.0);

    return uv;
}

////////////////////////////////////////////////////////////

void main()
{
    vec3 dir = normalize(fragPosition);

    // Choose mapping
    vec2 uv = (skyboxMode == 1)
        ? SampleCross(dir)
        : SampleSpherical(dir);

    if (vflipped == 1)
        uv.y = 1.0 - uv.y;

    vec3 color = texture(texture0, uv).rgb;

    // Basic color grading
    color *= exposure;
    color += brightness;
    color = (color - 0.5) * contrast + 0.5;

    // HDR tone mapping
    if (isHDR == 1)
        color = ACES(color);

    // Gamma correction
    if (doGamma == 1)
        color = pow(color, vec3(1.0 / fragGamma));

    vec4 background = vec4(color, 1.0);

    ////////////////////////////////////////////////////////////
    // Fog
    ////////////////////////////////////////////////////////////
    if (fogEnabled == 1)
    {
        // More fog near horizon
        float horizon = pow(1.0 - abs(dir.y), 3.0);

        // Moving mist
        float n = hash(dir * 5.0 + uTime * 0.2);

        float fogFactor = clamp(horizon + n * 0.3, 0.0, 1.0);
        fogFactor *= clamp(fogDensity * 5.0, 0.0, 1.0);

        finalColor = mix(background, fogColor, fogFactor);
    }
    else
    {
        finalColor = background;
    }
}