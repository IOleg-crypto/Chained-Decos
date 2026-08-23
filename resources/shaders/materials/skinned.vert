#version 430 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in ivec4 a_JointIDs;
layout(location = 4) in vec4 a_Weights;
layout(location = 5) in vec3 a_Tangent;

#include "../include/camera.glsl"

uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 boneMatrices[128];
uniform mat4 u_LightSpaceMatrix;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out mat3 fragTBN;
out vec4 fragPosLightSpace;

void main()
{
    vec3 vPos = a_Position;
    vec3 vNormal = a_Normal;
    vec3 vTangent = a_Tangent;

    float totalWeight = a_Weights.x + a_Weights.y + a_Weights.z + a_Weights.w;
    mat4 skinMat;
    if (totalWeight < 0.001)
    {
        skinMat = mat4(1.0);
    }
    else
    {
        skinMat = 
            (a_Weights.x * boneMatrices[a_JointIDs.x] +
             a_Weights.y * boneMatrices[a_JointIDs.y] +
             a_Weights.z * boneMatrices[a_JointIDs.z] +
             a_Weights.w * boneMatrices[a_JointIDs.w]) / totalWeight;
    }
    
    vPos = (skinMat * vec4(vPos, 1.0)).xyz;
    vNormal = (skinMat * vec4(vNormal, 0.0)).xyz;
    vTangent = (skinMat * vec4(vTangent, 0.0)).xyz;
    
    fragPosition = vec3(matModel * vec4(vPos, 1.0));
    fragTexCoord = a_TexCoord;
    
    fragColor = vec4(0.0, 0.0, 0.0, 0.0); 

    vec3 N = normalize(vec3(matNormal * vec4(vNormal, 0.0)));
    vec3 T;
        
    if (length(vTangent) < 0.01) 
    {
        vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        T = normalize(cross(up, N));
    } 
    else 
    {
        T = normalize(vec3(matNormal * vec4(vTangent, 0.0)));
    }
    
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    fragNormal = N;
    fragTBN = mat3(T, B, N);

    fragPosLightSpace = u_LightSpaceMatrix * vec4(fragPosition, 1.0);

    gl_Position = u_ViewProjection * matModel * vec4(vPos, 1.0);
}
