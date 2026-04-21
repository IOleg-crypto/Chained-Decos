#version 430 core

// Input from Vertex Shader
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;
in float windIntensity;

// Standard Engine Uniforms
uniform sampler2D texture0; // Albedo
uniform vec4 colDiffuse;
uniform int useTexture;
uniform vec3 viewPos;
uniform float uTime;

out vec4 finalColor;

void main()
{
    // 1. Resolve Base Color (Matching lighting.fs logic)
    vec4 texelColor = vec4(1.0);
    if (useTexture == 1)
    {
        texelColor = texture(texture0, fragTexCoord);
    }
    
    // Safety check for vertex color
    vec4 vColor = (length(fragColor.rgb) > 0.001) ? fragColor : vec4(1.0);
    vec4 baseColor = texelColor * colDiffuse * vColor;
    
    // 2. Apply Wind Effects (Intensified)
    if (windIntensity > 0.05)
    {
        // Rim Glow (Fresnel-like) - Much stronger power and intensity
        vec3 normal = normalize(fragNormal);
        vec3 viewDir = normalize(viewPos - fragPosition);
        float rim = 1.0 - max(dot(viewDir, normal), 0.0);
        rim = pow(rim, 2.0) * windIntensity; // Lower power for wider glow
        
        vec3 rimColor = vec3(0.4, 0.8, 1.0) * rim * 2.0; // Brighter blue
        
        // Scrolling Wind Lines - Faster and more dense
        float lines = sin(fragTexCoord.y * 150.0 - uTime * 60.0) * 0.5 + 0.5;
        lines *= sin(fragTexCoord.x * 40.0 + uTime * 10.0) * 0.5 + 0.5;
        lines = pow(lines, 8.0) * windIntensity * 3.0;
        
        vec3 windLines = vec3(1.0) * lines; 
        
        baseColor.rgb += rimColor + windLines;
        
        // Add a slight blue tint to the base during fall
        baseColor.rgb = mix(baseColor.rgb, vec3(0.0, 0.4, 0.8), windIntensity * 0.3);
    }

    finalColor = baseColor;
}
