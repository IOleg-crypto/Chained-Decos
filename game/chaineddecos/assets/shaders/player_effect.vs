#version 430 core

// Input vertex attributes matching engine's lighting.vs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in ivec4 a_JointIDs;
layout(location = 4) in vec4 a_Weights;

// Standard Engine Uniforms
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjection;
uniform mat4 matNormal;
uniform mat4 boneMatrices[128];
uniform int useSkinning;

// Custom Wind Effect Uniforms
uniform float fallSpeed;
uniform float uTime;
uniform vec3 windDirection;

// Output to Fragment Shader
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;
out float windIntensity;

void main()
{
    vec3 pos = a_Position;
    vec3 norm = a_Normal;

    // 1. Skinning Support
    if (useSkinning == 1)
    {
        mat4 skinMat = 
            a_Weights.x * boneMatrices[a_JointIDs.x] +
            a_Weights.y * boneMatrices[a_JointIDs.y] +
            a_Weights.z * boneMatrices[a_JointIDs.z] +
            a_Weights.w * boneMatrices[a_JointIDs.w];
        
        pos = (skinMat * vec4(pos, 1.0)).xyz;
        norm = (skinMat * vec4(norm, 0.0)).xyz;
    }

    // 2. Wind Deformation Logic (Intensified)
    // Starts reacting at 5.0, maxed out at 40.0
    float fallStrength = clamp((fallSpeed - 5.0) / 35.0, 0.0, 1.0);
    windIntensity = fallStrength;

    if (fallStrength > 0.01)
    {
        // Height factor (base of model moves less)
        float heightFactor = clamp(a_Position.y + 0.5, 0.0, 1.5);
        
        // Much more aggressive vibration
        float flutter = sin(uTime * 40.0 + pos.y * 15.0) * 0.05;
        float bigWave = sin(uTime * 15.0 + pos.z * 4.0) * 0.1;
        
        // Horizontal jitter
        vec3 jitter = vec3(
            sin(uTime * 60.0) * 0.02,
            0.0,
            cos(uTime * 55.0) * 0.02
        );
        
        vec3 windOffset = windDirection * (bigWave + flutter) * heightFactor * fallStrength;
        pos += windOffset + (jitter * heightFactor * fallStrength);
        
        // Stretching along wind direction
        pos += windDirection * heightFactor * fallStrength * 0.2;
    }

    // 3. Final Transformations
    fragPosition = vec3(matModel * vec4(pos, 1.0));
    fragNormal = normalize(mat3(matNormal) * norm);
    fragTexCoord = a_TexCoord;
    fragColor = vec4(1.0); 

    gl_Position = mvp * vec4(pos, 1.0);
}
