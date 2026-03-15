vec3 CalcDirectionalLight(vec3 lightDir, vec4 lightColor, vec3 normal, vec3 viewDir, vec3 baseColor, float shininess)
{
    vec3 L = normalize(-lightDir);
    float diff = max(dot(normal, L), 0.0);
    vec3 diffuseComp = baseColor * lightColor.rgb * diff;
    
    vec3 specularComp = vec3(0.0);
    if (diff > 0.0)
    {
        vec3 H = normalize(L + viewDir);
        float spec = pow(max(dot(normal, H), 0.0), max(1.0, shininess));
        specularComp = lightColor.rgb * spec;
    }
    
    return diffuseComp + specularComp;
}
