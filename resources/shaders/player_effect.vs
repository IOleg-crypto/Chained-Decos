#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjection;

// Wind effect parameters
uniform float fallSpeed;      // Fall speed (0-60, negative velocity.y)
uniform float time;            // Time for animation
uniform vec3 windDirection;   // Wind direction (e.g., vec3(1.0, 0.0, 0.5))

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPosition;
out vec4 fragColor;
out float windIntensity;      // Pass wind intensity to fragment shader

void main()
{
    vec3 pos = vertexPosition;
    vec3 normal = vertexNormal;
    
    // Apply wind effect only when player is falling
    if (fallSpeed > 0.5)  // Slightly higher threshold for activation
    {
        // Normalize fall speed (0-1)
        float fallStrength = clamp(fallSpeed / 40.0, 0.0, 1.0);
        
        // Wind intensity - subtle effect, no stretching
        // Use square for smoother increase
        float windStrength = fallStrength * fallStrength * 0.15; // Soft, natural effect
        
        // Vertex height relative to model center
        // Effect is stronger on upper parts (hood, hair, clothing)
        float heightFactor = max(0.0, vertexPosition.y);
        // Normalize height (assuming model height is about 2 units)
        heightFactor = smoothstep(0.0, 1.5, heightFactor); // Smooth transition function
        heightFactor = pow(heightFactor, 0.8); // Enhance effect on upper parts
        
        // Smooth wind animation - light oscillation (like natural wind gust)
        float windWave = sin(time * 3.0 + vertexPosition.x * 0.5 + vertexPosition.z * 0.3);
        float windWave2 = cos(time * 2.5 + vertexPosition.z * 0.4 - vertexPosition.x * 0.3);
        
        // Combine waves for natural movement
        float combinedWave = (windWave * 0.6 + windWave2 * 0.4);
        
        // Light vertex position offset (horizontal only, no stretching)
        // Effect is proportional to height - upper parts move more
        vec3 windOffset = windDirection * combinedWave * windStrength * heightFactor;
        
        // Minimal horizontal offset - only light movement (like clothing sway)
        pos.x += windOffset.x;
        pos.z += windOffset.z;
        
        // Light normal bending for better lighting under wind
        // This creates light reflection effect from clothing under wind
        normal = normalize(normal + windOffset * 0.3);
        
        // Pass wind intensity to fragment shader for visual effects
        windIntensity = windStrength * heightFactor;
    }
    else
    {
        windIntensity = 0.0;
    }
    
    // Transform position to world coordinates
    fragPosition = vec3(matModel * vec4(pos, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(mat3(matModel) * normal);
    
    // Final screen position
    gl_Position = matProjection * matView * vec4(fragPosition, 1.0);
}
