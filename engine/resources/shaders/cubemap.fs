#version 450 core

layout(location = 0) in vec3 fragPosition;

layout(binding = 0) uniform sampler2D equirectangularMap;

layout(location = 0) out vec4 finalColor;

#define PI 3.14159265358979323846

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183); // 1/(2*PI), 1/PI
    uv += 0.5;
    return uv;
}

void main()
{		
    vec3 v = normalize(fragPosition);
    vec2 uv = SampleSphericalMap(v);
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    finalColor = vec4(color, 1.0);
}
