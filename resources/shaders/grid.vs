#version 330 core

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;

// Output vertex attributes (to fragment shader)
out vec3 fragWorldPos;

void main()
{
    fragWorldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
