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

    // Final color
    finalColor = (ambientColor + diffuseColor) * texelColor;
}
