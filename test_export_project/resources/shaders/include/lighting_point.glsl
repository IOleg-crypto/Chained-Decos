// ============================================================
// Point Light — Blinn-Phong + Schlick Fresnel + Physical Attenuation
// ============================================================

// Inverse-square attenuation with smooth windowing at radius boundary.
// Physical: intensity / d^2, with a smooth fade in the last 20% of radius.
float PointLightAttenuation(float distance, float radius)
{
    if (distance >= radius) return 0.0;

    float attenuation = 1.0 / (1.0 + distance * distance);

    // Smooth fade in the last 20% of radius
    float fadeStart = radius * 0.8;
    if (distance > fadeStart)
    {
        float t = (distance - fadeStart) / (radius - fadeStart);
        attenuation *= 1.0 - t * t;
    }

    return attenuation;
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir,
                    vec3 diffuseColor, vec3 specularColor, float shininess)
{
    if (light.enabled == 0) return vec3(0.0);

    vec3 lightVector = light.position - fragPos;
    float distance = length(lightVector);

    float attenuation = PointLightAttenuation(distance, light.radius);
    if (attenuation <= 0.0) return vec3(0.0);

    vec3 L = normalize(lightVector);
    float NdotL = max(dot(normal, L), 0.0);

    // Diffuse: Lambertian
    vec3 diffuse = diffuseColor * light.color.rgb * NdotL;

    vec3 specular = vec3(0.0);
    if (NdotL > 0.0)
    {
        vec3 H = normalize(L + viewDir);
        float NdotH = max(dot(normal, H), 0.0);
        float HdotV = max(dot(H, viewDir), 0.0);

        // Blinn-Phong specular
        float spec = pow(NdotH, max(1.0, shininess));

        // Schlick Fresnel
        float fresnel = pow(1.0 - HdotV, 5.0);
        vec3 F = specularColor + (vec3(1.0) - specularColor) * fresnel;

        specular = F * light.color.rgb * spec;
    }

    return (diffuse + specular) * attenuation * light.intensity;
}
