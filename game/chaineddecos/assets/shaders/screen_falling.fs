#version 330 core

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float time;
uniform float intensity; // 0.0 to 1.0
uniform vec3 color;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    if (intensity < 0.01) discard;

    vec2 uv = fragTexCoord;
    
    // Vertical speed lines
    float lineCount = 100.0;
    float x = floor(uv.x * lineCount);
    
    // Randomize speed and offset for each line
    float randVal = random(vec2(x, 123.0));
    float speed = 5.0 + randVal * 10.0;
    float offset = randVal * 10.0;
    
    // Calculate line pattern
    float y = uv.y * 5.0 + time * speed + offset;
    float line = step(0.9, fract(y * (0.5 + randVal)));
    
    // Fade at the edges of the screen
    float edgeFade = smoothstep(0.0, 0.2, uv.y) * smoothstep(1.0, 0.8, uv.y);
    
    float alpha = line * intensity * edgeFade * (0.1 + randVal * 0.4);
    
    finalColor = vec4(color, alpha);
}
