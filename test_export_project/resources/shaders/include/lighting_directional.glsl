// ============================================================
// Directional Light — Blinn-Phong + Schlick Fresnel
// ============================================================

// Wrap factor for half-Lambert diffuse.
// With WRAP=0.15 the darkest surface (facing directly away from the light)
// still receives 15% of full diffuse — preventing pitch-black shadows.
const float WRAP = 0.15;

vec3 CalcDirectionalLight(vec3 lightDir, vec4 lightColor, vec3 normal, vec3 viewDir,
                          vec3 diffuseColor, vec3 specularColor, float shininess)
{
    vec3 L = normalize(-lightDir);

    // Half-Lambert wrap: map NdotL from [-1,1] → [~0,1] with smooth rolloff.
    // NdotL=0  (grazing) → wrappedNdotL=WRAP (still a little lit).
    // NdotL=-1 (back face) → wrappedNdotL=0  (ambient takes over).
    float rawNdotL     = dot(normal, L);
    float wrappedNdotL = clamp(rawNdotL * (1.0 - WRAP) + WRAP, 0.0, 1.0);

    // Diffuse: Lambertian with wrap
    vec3 diffuse = diffuseColor * lightColor.rgb * wrappedNdotL;

    vec3 specular = vec3(0.0);
    if (rawNdotL > 0.0)
    {
        vec3 H = normalize(L + viewDir);
        float NdotH = max(dot(normal, H), 0.0);
        float HdotV = max(dot(H, viewDir), 0.0);

        // Blinn-Phong specular
        float spec = pow(NdotH, max(1.0, shininess));

        // Schlick Fresnel: F = F0 + (1 - F0) * (1 - HdotV)^5
        float fresnel = pow(1.0 - HdotV, 5.0);
        vec3 F = specularColor + (vec3(1.0) - specularColor) * fresnel;

        specular = F * lightColor.rgb * spec;
    }

    return diffuse + specular;
}
