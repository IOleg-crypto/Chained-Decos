// ============================================================
// Spot Light — Blinn-Phong + Schlick Fresnel + Physical Attenuation
// ============================================================

float SpotLightAttenuation(float distance, float radius)
{
    if (distance >= radius) return 0.0;

    float attenuation = 1.0 / (1.0 + distance * distance);

    float fadeStart = radius * 0.8;
    if (distance > fadeStart)
    {
        float t = (distance - fadeStart) / (radius - fadeStart);
        attenuation *= 1.0 - t * t;
    }

    return attenuation;
}

vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir,
                   vec3 diffuseColor, vec3 specularColor, float shininess)
{
    if (light.enabled == 0) return vec3(0.0);

    vec3 lightVector = light.position - fragPos;
    float distance = length(lightVector);

    float attenuation = SpotLightAttenuation(distance, light.radius);
    if (attenuation <= 0.0) return vec3(0.0);

    vec3 L = normalize(lightVector);
    float cosTheta = dot(L, normalize(-light.direction));

    float epsilon = cos(radians(light.innerCutoff)) - cos(radians(light.outerCutoff));
    float spotIntensity = clamp((cosTheta - cos(radians(light.outerCutoff))) / epsilon, 0.0, 1.0);

    if (spotIntensity <= 0.0) return vec3(0.0);

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

    return (diffuse + specular) * attenuation * light.intensity * spotIntensity;
}
