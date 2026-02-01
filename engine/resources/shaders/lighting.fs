#version 430

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// Light data
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform float ambient;

#define MAX_LIGHTS 8

struct Light {
    vec3 position;
    vec4 color;
    float radius;
    float radiance;
    float falloff;
    int enabled;
};

uniform Light lights[MAX_LIGHTS];

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 diffuse = colDiffuse * fragColor;

    // Calculate ambient light
    vec4 ambientColor = diffuse * ambient;

    // Calculate diffuse light (directional)
    float diff = max(dot(fragNormal, normalize(-lightDir)), 0.0);
    vec4 diffuseColor = diffuse * lightColor * diff;

    // Point lights
    vec4 pointLightsColor = vec4(0.0);
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 0) continue;

        vec3 lightVector = lights[i].position - fragPosition;
        float distance = length(lightVector);
        
        if (distance < lights[i].radius)
        {
            vec3 L = normalize(lightVector);
            float NdotL = max(dot(fragNormal, L), 0.0);
            
            // Basic falloff
            float attenuation = pow(clamp(1.0 - distance/lights[i].radius, 0.0, 1.0), lights[i].falloff);
            
            pointLightsColor += diffuse * lights[i].color * NdotL * attenuation * lights[i].radiance;
        }
    }

    // Final color
    finalColor = (ambientColor + diffuseColor + pointLightsColor) * texelColor;
}
