#version 430 core

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // Color buffer
uniform sampler2D texture1; // Depth buffer

uniform mat4 matInverseViewProj;
uniform vec3 viewPos;
uniform float uTime;

uniform float uExposure;
uniform float uGamma;

// Helper for dithering (removes banding)
float InterleavedGradientNoise(vec2 uv)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

void main()
{
    vec4 color = texture(texture0, fragTexCoord);
    float depth = texture(texture1, fragTexCoord).r;

    // 1. Reconstruct World Position from depth
    float z = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(fragTexCoord * 2.0 - 1.0, z, 1.0);
    vec4 worldPos = matInverseViewProj * clipPos;
    worldPos /= worldPos.w;

    // 2. Tonemapping (ACES Filmic)
    float exposure = (uExposure > 0.0) ? uExposure : 1.0;
    vec3 outColor = color.rgb * exposure;
    
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    outColor = clamp((outColor * (a * outColor + b)) / (outColor * (c * outColor + d) + e), 0.0, 1.0);
    
    // 3. Dithering (smooth gradients)
    float dither = InterleavedGradientNoise(floor(gl_FragCoord.xy));
    outColor += (dither - 0.5) / 255.0;

    // 4. Gamma Correction
    float gamma = (uGamma > 0.0) ? uGamma : 2.2;
    outColor = pow(outColor, vec3(1.0 / gamma));

    finalColor = vec4(outColor, color.a);
}
