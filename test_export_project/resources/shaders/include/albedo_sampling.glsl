// Albedo/Base Color Sampling


vec4 SampleAlbedo(in vec2 texCoord, 
                  in vec4 colorMultiplier,
                  in float useTexture)
{
    
    vec4 baseColor = colDiffuse;
    
    
    if (length(colorMultiplier.rgb) > 0.01) 
    {
        baseColor *= colorMultiplier;
    }
    
    
    if (useTexture == 1) 
    {
        vec4 sampled = texture(texture0, texCoord);
        baseColor.rgb *= ToLinear(sampled.rgb); 
        baseColor.a   *= sampled.a;
    }
    
    return baseColor;
}


bool ShouldDiscardPixel(in float alpha, in float threshold)
{
    return alpha < threshold;
}
