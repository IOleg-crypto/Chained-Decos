vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, float shininess)
{
    if (light.enabled == 0) return vec3(0.0);

    vec3 lightVector = light.position - fragPos;
    float distance = length(lightVector);
    
    if (distance >= light.radius) return vec3(0.0);

    vec3 L = normalize(lightVector);
    float NdotL = max(dot(normal, L), 0.0);
    
    // Attenuation
    float attenuation = max(0.0, 1.0 - (distance / light.radius));
    attenuation = pow(attenuation, 1.5);
    
    // Diffuse
    vec3 p_diff = baseColor * light.color.rgb * NdotL;
    
    // Specular
    vec3 p_spec = vec3(0.0);
    if (NdotL > 0.0)
    {
        vec3 H = normalize(L + viewDir);
        float spec = pow(max(dot(normal, H), 0.0), max(1.0, shininess));
        p_spec = light.color.rgb * spec;
    }
    
    return (p_diff + p_spec) * attenuation * light.intensity;
}
