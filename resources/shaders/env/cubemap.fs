#version 430 core

in vec3 fragPosition;

uniform sampler2D equirectangularMap;

out vec4 finalColor;

const vec2 invAtan = vec2(0.1591, 0.3183); // 1/(2*PI) and 1/PI

vec2 SampleSphericalMap(vec3 v)
{
    // atan2 returns values from -PI to PI
    // asin returns from -PI/2 to PI/2
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    
    // Convert to [0, 1] range
    uv *= invAtan;
    uv += 0.5;
    
    // stb_image loads JPG/PNG with V-flip (stbi_set_flip_vertically_on_load).
    // Compensate here so cubemap generates correctly.
    uv.y = 1.0 - uv.y;
    
    return uv;
}

void main()
{       
    vec3 direction = normalize(fragPosition);
    vec2 uv = SampleSphericalMap(direction);
    
    // IMPORTANT: textureLod removes seams at wrapping boundaries
    // by ignoring mipmap gradients.
    vec3 color = textureLod(equirectangularMap, uv, 0.0).rgb;
    
    finalColor = vec4(color, 1.0);
}