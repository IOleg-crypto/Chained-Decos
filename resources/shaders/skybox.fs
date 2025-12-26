#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
// Input uniform values
uniform samplerCube environmentMap;
uniform bool vflipped;
uniform bool doGamma;
uniform float fragGamma;
uniform float exposure;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Fetch color from texture map
    vec3 color = vec3(0.0);

    if (vflipped){
        color = texture(environmentMap, vec3(fragPosition.x, -fragPosition.y, fragPosition.z)).rgb;
    }
    else {
        color = texture(environmentMap, fragPosition).rgb;
    }

    // Apply exposure
    color *= exposure;

    if (doGamma)// Apply gamma correction
    {
    
        color = pow(color, vec3(1.0/fragGamma));
    }

    // Calculate final fragment color
    finalColor = vec4(color, 1.0);
}