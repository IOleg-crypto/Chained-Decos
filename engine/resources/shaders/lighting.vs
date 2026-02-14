#version 430

#define MAX_BONES 128

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec4 vertexBoneIds;
in vec4 vertexBoneWeights;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 boneMatrices[MAX_BONES];

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

// Main entry point
void main()
{
    vec3 vPos = vertexPosition;
    vec3 vNormal = vertexNormal;

    // Auto-detect skinning based on weights
    float totalWeight = vertexBoneWeights.x + vertexBoneWeights.y + vertexBoneWeights.z + vertexBoneWeights.w;
    if (totalWeight > 0.01)
    {
        mat4 skinMat = 
            vertexBoneWeights.x * boneMatrices[int(vertexBoneIds.x)] +
            vertexBoneWeights.y * boneMatrices[int(vertexBoneIds.y)] +
            vertexBoneWeights.z * boneMatrices[int(vertexBoneIds.z)] +
            vertexBoneWeights.w * boneMatrices[int(vertexBoneIds.w)];
        
        vPos = (skinMat * vec4(vertexPosition, 1.0)).xyz;
        vNormal = (skinMat * vec4(vertexNormal, 0.0)).xyz;
    }

    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel * vec4(vPos, 1.0));
    fragTexCoord = vertexTexCoord;
    
    // Default to white if vertex color is absent or black with zero alpha
    if (vertexColor.a > 0.0) fragColor = vertexColor;
    else fragColor = vec4(1.0, 1.0, 1.0, 1.0);

    fragNormal = normalize(vec3(matNormal * vec4(vNormal, 0.0)));

    // Calculate final vertex position
    gl_Position = mvp * vec4(vPos, 1.0);
}
