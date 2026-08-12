#version 430 core

in vec2 TexCoords;

out vec4 finalColor;

uniform sampler2D textTexture;
uniform vec4 textColor;
uniform vec4 backgroundColor;
uniform float useBackground;

void main()
{
    vec4 texColor = texture(textTexture, TexCoords);

    if (useBackground > 0.5)
    {
        // Text with semi-transparent background
        vec3 bg = backgroundColor.rgb;
        float bgAlpha = backgroundColor.a * (1.0 - texColor.a);
        float textAlpha = texColor.a * textColor.a;
        finalColor = vec4(
            mix(bg, textColor.rgb, texColor.a),
            max(bgAlpha, textAlpha)
        );
    }
    else
    {
        // Text only (transparent background)
        finalColor = vec4(textColor.rgb, texColor.a * textColor.a);
    }
}
