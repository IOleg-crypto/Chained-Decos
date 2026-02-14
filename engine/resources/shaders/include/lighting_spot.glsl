vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, float shininess)
{
    if (light.enabled == 0) return vec3(0.0);

    vec3 lightVector = light.position - fragPos;
    float distance = length(lightVector);

    if (distance >= light.range) return vec3(0.0);

    vec3 L = normalize(lightVector);
    float cosTheta = dot(L, normalize(-light.direction));
    
    float epsilon = cos(radians(light.innerCutoff)) - cos(radians(light.outerCutoff));
    float spotIntensity = clamp((cosTheta - cos(radians(light.outerCutoff))) / epsilon, 0.0, 1.0);

    if (spotIntensity <= 0.0) return vec3(0.0);

    float NdotL = max(dot(normal, L), 0.0);
    
    // Attenuation
    float attenuation = max(0.0, 1.0 - (distance / light.range));
    attenuation = pow(attenuation, 1.5);
    
    // Diffuse
    vec3 s_diff = baseColor * light.color.rgb * NdotL;
    
    // Specular
    vec3 s_spec = vec3(0.0);
    if (NdotL > 0.0)
    {
        vec3 H = normalize(L + viewDir);
        float spec = pow(max(dot(normal, H), 0.0), max(1.0, shininess));
        s_spec = light.color.rgb * spec;
    }
    
    return (s_diff + s_spec) * attenuation * light.intensity * spotIntensity;
}
