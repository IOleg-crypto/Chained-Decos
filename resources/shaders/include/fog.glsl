// ============================================================


// ============================================================

uniform int   fogEnabled;
uniform int   fogMode;
uniform vec4  fogColor;         
uniform float fogDensity;       
uniform float fogStart;         
uniform float fogEnd;           
uniform float fogHeightFalloff; 

// ════════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════════



float GetHeightFogFactor(float worldHeight)
{
    
    
    return exp(-worldHeight * fogHeightFalloff);
}


float LinearFog(float distance)
{
    float range = fogEnd - fogStart;
    return clamp((distance - fogStart) / max(0.0001, range), 0.0, 1.0);
}


float ExponentialFog(float distance)
{
    float d = max(0.0, distance - fogStart);
    return 1.0 - exp(-d * fogDensity);
}


float ExponentialSquaredFog(float distance)
{
    float d = max(0.0, distance - fogStart) * fogDensity;
    return 1.0 - exp(-(d * d));
}


float CombinedFog(float distance, float heightFactor)
{
    
    float distanceFog = 0.0;
    if (fogMode == 0) // Linear
    {
        distanceFog = LinearFog(distance);
    }
    else if (fogMode == 1) // Exponential
    {
        distanceFog = ExponentialFog(distance);
    }
    else if (fogMode == 2) // Exponential Squared
    {
        distanceFog = ExponentialSquaredFog(distance);
    }
    
    
    return mix(distanceFog, 0.0, heightFactor);
}



vec3 ComputeFogScattering(vec3 viewDir, vec3 lightDir, vec3 lightColor, float sunIntensity)
{
    
    float cosTheta = -dot(viewDir, normalize(lightDir));
    
    
    
    float phaseRayleigh = 0.75 * (1.0 + cosTheta * cosTheta);
    
    
    float forwardScatter = pow(max(0.0, cosTheta), 8.0) * sunIntensity;
    
    
    return lightColor * (phaseRayleigh * 0.3 + forwardScatter * 0.7);
}



float HorizonFogBoost(vec3 viewDir)
{
    
    float upness = abs(viewDir.y);
    
    return 1.0 + (1.0 - upness) * 0.5;
}

// ════════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════════

vec3 ApplyLinearFog(vec3 surfaceColor, vec3 fragPos, vec3 viewPos, vec3 lightDir, vec3 lightColor)
{
    if (fogEnabled == 0) 
        return surfaceColor;

    
    vec3 viewDir = normalize(viewPos - fragPos);
    float distance = length(viewPos - fragPos);
    
    
    
    float worldHeight = fragPos.y;
    float heightFactor = GetHeightFogFactor(worldHeight);
    
    
    float fogFactor = CombinedFog(distance, heightFactor);
    
    
    float horizonBoost = HorizonFogBoost(viewDir);
    fogFactor *= horizonBoost;
    
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    
    vec3 baseFogLinear = pow(fogColor.rgb, vec3(2.2));

    
    
    float sunIntensity = 2.0; 
    vec3 scatteredLight = ComputeFogScattering(viewDir, lightDir, lightColor, sunIntensity);
    
    
    vec3 finalFogColor = baseFogLinear + scatteredLight * 0.3;

    
    return mix(surfaceColor, finalFogColor, fogFactor);
}

// ════════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════════
vec4 ApplyFog(vec4 surfaceColor, vec3 fragPos, vec3 viewPos, float uTime)
{
    vec3 fogRGB = ApplyLinearFog(surfaceColor.rgb, fragPos, viewPos, vec3(0.0, -1.0, 0.0), vec3(0.5));
    return vec4(fogRGB, surfaceColor.a);
}