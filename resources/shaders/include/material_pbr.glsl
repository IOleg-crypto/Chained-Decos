// ============================================================
// Material Properties — Blinn-Phong + Fresnel
// ============================================================

// ── Material property calculation ─────────────────────────
void CalculateMaterialProperties(in vec2 texCoord,
                                  out float metalness_out,
                                  out float roughness_out,
                                  out float occlusion_out)
{
    metalness_out = metalness;
    roughness_out = roughness;
    occlusion_out = 1.0;

    if (useMetallicMap == 1)
        metalness_out *= texture(texture1, texCoord).b;

    if (useRoughnessMap == 1)
        roughness_out *= texture(texture1, texCoord).g;

    if (useOcclusionMap == 1)
        occlusion_out = texture(texture4, texCoord).r;

    metalness_out = clamp(metalness_out, 0.0, 1.0);
    roughness_out = clamp(roughness_out, 0.04, 1.0);
}

// ── Specular properties (Blinn-Phong + Fresnel) ───────────
// shininess derived from roughness (lower roughness = tighter highlight)
// specularColor uses F0: dielectric 0.04, metals tinted by albedo
// diffuseColor = albedo * (1 - metalness)
void CalculateSpecularProperties(in float metalness,
                                  in float roughness,
                                  in vec3 baseColor,
                                  out float shininess,
                                  out vec3 specularColor,
                                  out vec3 diffuseColor)
{
    shininess = (1.0 - roughness) * 128.0;
    if (shininess < 1.0) shininess = 1.0;

    specularColor = mix(vec3(0.04), baseColor, metalness);
    diffuseColor  = baseColor * (1.0 - metalness);
}
