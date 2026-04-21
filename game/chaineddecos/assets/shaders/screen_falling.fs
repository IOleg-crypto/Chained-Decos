#version 330 core

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float uTime;
uniform float intensity; // 0.0 to 1.0 - controlled by PlayerFall.cs
uniform vec3 color;
uniform sampler2D texture0; // Scene color texture

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    vec2 uv = fragTexCoord;

    // Sample scene first
    vec4 sceneColor = texture(texture0, uv);

    // If no effect needed, just output scene
    if (intensity < 0.001)
    {
        finalColor = sceneColor;
        return;
    }

    // Vertical speed lines pattern
    float lineCount = 80.0;
    float x = floor(uv.x * lineCount);

    // Each line has a random speed and offset
    float randVal = random(vec2(x, 73.5));
    float speed = 3.0 + randVal * 8.0;
    float offset = randVal * 10.0;

    // Animate downward
    float y = uv.y * 4.0 + uTime * speed + offset;
    float line = step(0.88, fract(y * (0.4 + randVal * 0.3)));

    // Fade at top/bottom edges
    float edgeFade = smoothstep(0.0, 0.15, uv.y) * smoothstep(1.0, 0.85, uv.y);

    // Compute line alpha
    float lineAlpha = line * edgeFade * (0.15 + randVal * 0.5);

    // Final alpha is intensity-driven
    float alpha = lineAlpha * intensity;

    // Mix over scene
    finalColor = vec4(mix(sceneColor.rgb, color, alpha), 1.0);
}
