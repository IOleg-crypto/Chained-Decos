#version 330 core

// Input fragment attributes (from vertex shader)
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;
in vec4 fragColor;
in float windIntensity;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Get color from model texture
    vec4 texelColor = texture(texture0, fragTexCoord);
    
    // Base color
    vec4 baseColor = texelColor * colDiffuse * fragColor;
    
    // Subtle visual wind effect - light clothing illumination
    if (windIntensity > 0.01)
    {
        // Light illumination effect from movement (clothing reflects light differently under wind)
        // Use normal to calculate wind direction
        vec3 lightDir = normalize(vec3(1.0, -0.5, 0.5)); // Light direction
        float windLight = max(0.0, dot(fragNormal, lightDir));
        
        // Light highlight on parts moving under wind
        float windHighlight = windIntensity * windLight * 0.15;
        baseColor.rgb += windHighlight * vec3(0.1, 0.1, 0.15); // Light blue tint
    }
    
    finalColor = baseColor;
}
