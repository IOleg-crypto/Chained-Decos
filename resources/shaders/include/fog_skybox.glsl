#ifndef CH_FOG_SKYBOX_GLSL
#define CH_FOG_SKYBOX_GLSL

// Minimal fog uniforms shared by all skybox fragment shaders.
// Skybox fog is horizon-based (not distance-based like mesh fog in fog.glsl).
uniform int   fogEnabled;
uniform vec4  fogColor;
uniform float fogDensity;

#endif // CH_FOG_SKYBOX_GLSL
