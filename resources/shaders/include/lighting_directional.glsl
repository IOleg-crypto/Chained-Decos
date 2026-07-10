// ============================================================
// Directional Light — Blinn-Phong + Schlick Fresnel
// ============================================================
vec3 CalcDirectionalLight(vec3 lightDir, vec4 lightColor, vec3 normal, vec3 viewDir,
                          vec3 diffuseColor, vec3 specularColor, float shininess)
{
    vec3 L = normalize(-lightDir);
    float NdotL = max(dot(normal, L), 0.0);

    // Diffuse: Lambertian
    vec3 diffuse = diffuseColor * lightColor.rgb * NdotL;

    vec3 specular = vec3(0.0);
    if (NdotL > 0.0)
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
