#version 330

in vec3 fragTexCoord;

out vec4 finalColor;

uniform samplerCube texture0;

void main()
{
    finalColor = texture(texture0, fragTexCoord);
}
