#ifndef CH_COLOR_SPACE_GLSL
#define CH_COLOR_SPACE_GLSL

// Converts sRGB color (Gamma 2.2) to Linear space
vec3 ToLinear(vec3 srgb)
{
    return pow(srgb, vec3(2.2));
}

vec4 ToLinear(vec4 srgb)
{
    return vec4(pow(srgb.rgb, vec3(2.2)), srgb.a);
}

// Converts Linear color to sRGB space (Gamma 2.2)
vec3 ToSRGB(vec3 linear)
{
    return pow(linear, vec3(1.0 / 2.2));
}

#endif // CH_COLOR_SPACE_GLSL
