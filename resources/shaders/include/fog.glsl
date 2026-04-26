// ============================================================
//  fog.glsl  –  World-space fog include
//
//  Usage: ApplyFog(color, fragPos, viewPos, time)
//    color   – vec4 already-lit fragment color
//    fragPos – vec3 fragment position in WORLD space
//    viewPos – vec3 camera position in WORLD space
//    time    – float uTime (seconds), used by animated modes
//
//  Fog modes (set via fogMode uniform):
//    0 – Linear       : lerps from fogStart to fogEnd
//    1 – Exponential  : classic exp fog
//    2 – Exp Squared  : denser / more realistic exp fog
// ============================================================

// --- Uniforms exposed to the renderer ---
uniform int   fogEnabled;
uniform int   fogMode;      // 0=Linear, 1=Exp, 2=Exp2
uniform vec4  fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

// --- Single entry point ---
vec4 ApplyFog(vec4 color, vec3 fragPos, vec3 viewPos, float time)
{
    // Early-out: fog is disabled
    if (fogEnabled == 0) return color;

    // 1. Distance from camera to fragment (both in World Space)
    float dist = length(viewPos - fragPos);

    // 2. Compute fogFactor (0 = no fog, 1 = full fog)
    float fogFactor = 0.0;

    if (fogMode == 0) // ── Linear ─────────────────────────────
    {
        // fogFactor = (dist - start) / (end - start)
        float range = fogEnd - fogStart;
        if (range <= 0.0)
            fogFactor = (dist > fogStart) ? 1.0 : 0.0;
        else
            fogFactor = (dist - fogStart) / range;
    }
    else if (fogMode == 1) // ── Exponential ────────────────────
    {
        // fogFactor = 1 - e^(-dist * density)
        // Add max(0, dist - fogStart) so fog doesn't start directly at the camera
        float d = max(0.0, dist - fogStart);
        fogFactor = 1.0 - exp(-(d * fogDensity));
    }
    else if (fogMode == 2) // ── Exponential Squared ────────────
    {
        // fogFactor = 1 - e^(-(dist * density)^2)
        float d = max(0.0, dist - fogStart) * fogDensity;
        fogFactor = 1.0 - exp(-(d * d));
    }

    // 3. Clamp to [0, 1]
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // 4. Mix object color with fog color
    //    mix(a, b, t) = a*(1-t) + b*t
    return mix(color, fogColor, fogFactor);
}
