#version 430 core

in vec2 fragTexCoord;
in vec3 fragPosition;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    finalColor = colDiffuse * texColor;
}
