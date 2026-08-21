// Normal Map Sampling & Calculation


vec3 CalculateNormal(in vec3 vertexNormal,
                     in mat3 tbn,
                     in vec2 texCoord,
                     in float useNormalMap)
{
    vec3 normal = normalize(vertexNormal);
    
    if (useNormalMap == 1) 
    {
        vec3 mapNormal = texture(texture2, texCoord).rgb;
        mapNormal = normalize(mapNormal * 2.0 - 1.0);
        normal = normalize(tbn * mapNormal);
    }
    
    return normal;
}


vec4 DebugNormals(in vec3 normal, in float alpha)
{
    
    return vec4(normal * 0.5 + 0.5, alpha);
}
