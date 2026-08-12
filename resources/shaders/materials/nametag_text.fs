#version 430 core

in vec2 TexCoords;

out vec4 finalColor;

uniform sampler2D fontAtlas;
uniform vec4 textColor;

void main()
{
    float alpha = texture(fontAtlas, TexCoords).a;
    finalColor = vec4(textColor.rgb, textColor.a * alpha);
}
