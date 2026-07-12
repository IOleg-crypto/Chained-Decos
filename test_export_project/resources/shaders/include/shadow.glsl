// shadow.glsl — Shadow mapping utilities
//
// How it works:
//   1. The ShadowPass renders the scene from the light's point of view into a depth texture.
//   2. In the lighting shader, we transform each fragment into light-space.
//   3. We compare the fragment's depth against the shadow map depth.
//   4. If the fragment is farther from the light than what's stored in the map, it's in shadow.
//
// Uniforms (set from C++):
//   sampler2D u_ShadowMap    — the depth texture from the ShadowPass
//   mat4      u_LightSpaceMatrix — transforms world coords to light clip space
//   float     u_ShadowBias  — offset to prevent shadow acne (self-shadowing artifacts)
//   int       u_ShadowsEnabled — 0 = no shadows, 1 = shadows on

uniform sampler2D u_ShadowMap;
uniform mat4 u_LightSpaceMatrix;
uniform float u_ShadowBias;
uniform int u_ShadowsEnabled;

// Converts a world-space position to shadow map coordinates.
// Returns vec3(x, y, z) where x,y are UV coords [0,1] and z is the depth.
vec3 ShadowMapCoords(vec4 fragPosLightSpace)
{
    // Perspective divide: convert from clip space to NDC [-1, +1]
    vec3 ndc = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform from NDC [-1,+1] to texture coordinates [0,1]
    // x and y become UV coords, z is the depth we compare against
    return vec3(ndc.x * 0.5 + 0.5, ndc.y * 0.5 + 0.5, ndc.z * 0.5 + 0.5);
}

// Basic shadow test: returns 0.0 (fully in shadow) or 1.0 (fully lit).
float ShadowCalculationBasic(vec4 fragPosLightSpace)
{
    if (u_ShadowsEnabled == 0) return 1.0;

    vec3 coords = ShadowMapCoords(fragPosLightSpace);

    // Fragments behind the light's far plane are not in shadow
    if (coords.z > 1.0) return 1.0;

    // Get the closest depth from the shadow map
    float closestDepth = texture(u_ShadowMap, coords.xy).r;

    // Current fragment's depth (with bias to prevent acne)
    float currentDepth = coords.z - u_ShadowBias;

    // Compare: is this fragment farther than what the shadow map recorded?
    return currentDepth > closestDepth ? 0.0 : 1.0;
}

// PCF (Percentage-Closer Filtering) with Poisson Disk sampling: soft edges without visible banding.
float ShadowCalculationPCF(vec4 fragPosLightSpace)
{
    if (u_ShadowsEnabled == 0) return 1.0;

    vec3 coords = ShadowMapCoords(fragPosLightSpace);

    // Fragments behind the light's far plane or outside frustum are not in shadow
    if (coords.z > 1.0 || coords.x < 0.0 || coords.x > 1.0 || coords.y < 0.0 || coords.y > 1.0)
        return 1.0;

    float shadow = 0.0;
    float currentDepth = coords.z - u_ShadowBias;

    // Fixed Poisson disk for 16 samples
    const vec2 poissonDisk[16] = vec2[](
        vec2( -0.94201624, -0.39906216 ),
        vec2( 0.94558609, -0.76890725 ),
        vec2( -0.094184101, -0.92938870 ),
        vec2( 0.34495938, 0.29387760 ),
        vec2( -0.91588581, 0.45771432 ),
        vec2( -0.81544232, -0.87912464 ),
        vec2( -0.38277543, 0.27676845 ),
        vec2( 0.97484398, 0.75648379 ),
        vec2( 0.44323325, -0.97511554 ),
        vec2( 0.53742981, -0.47373420 ),
        vec2( -0.26496911, -0.41893023 ),
        vec2( 0.79197514, 0.19090188 ),
        vec2( -0.24188840, 0.99706507 ),
        vec2( -0.81409955, 0.91437590 ),
        vec2( 0.19984126, 0.78641367 ),
        vec2( 0.14383161, -0.14100790 )
    );

    // Spread radius (adjustable for softer shadows)
    float filterRadius = 2.0 / float(textureSize(u_ShadowMap, 0).x);

    // Sample the shadow map
    for (int i = 0; i < 16; ++i)
    {
        float pcfDepth = texture(u_ShadowMap, coords.xy + poissonDisk[i] * filterRadius).r;
        shadow += currentDepth > pcfDepth ? 0.0 : 1.0;
    }

    return shadow / 16.0;
}
