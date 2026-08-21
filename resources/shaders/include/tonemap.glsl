// Tonemapping & Gamma Correction



vec3 Tonemap(in vec3 linearColor, in float exposure)
{
    
    vec3 exposed = vec3(1.0) - exp(-linearColor * exposure);
    return exposed;
}


vec3 GammaCorrect(in vec3 srgbColor)
{
    
    return ToSRGB(srgbColor);
}


vec3 FinalizeColor(in vec3 linearColor, 
                   in float exposure, 
                   in float gamma)
{
    
    vec3 tonemapped = Tonemap(linearColor, exposure);
    
    
    vec3 output_color = GammaCorrect(tonemapped);
    
    return output_color;
}
