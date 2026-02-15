#version 330 core

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec4 vertexTangent;
in vec4 vertexBoneIds;
in vec4 vertexBoneWeights;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 boneMatrices[128]; // Use explicit constant to avoid version dependency issues

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out mat3 fragTBN;

void main()
{
    vec3 vPos = vertexPosition;
    vec3 vNormal = vertexNormal;

    // Auto-detect skinning: active if we have weights in the first three slots 
    // (Static meshes often default to 0,0,0,1 for unassigned attributes in GL)
    float activeWeights = vertexBoneWeights.x + vertexBoneWeights.y + vertexBoneWeights.z;
    if (activeWeights > 0.01)
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
    
    if (vertexColor.a > 0.0) fragColor = vertexColor;
    else fragColor = vec4(1.0, 1.0, 1.0, 1.0);

    vec3 N = normalize(vec3(matNormal * vec4(vNormal, 0.0)));
    
    // Default TBN if no tangents provided
    vec3 T = vec3(0.0);
    vec3 B = vec3(0.0);
    
    if (length(vertexTangent.xyz) > 0.01)
    {
        T = normalize(vec3(matNormal * vec4(vertexTangent.xyz, 0.0)));
        T = normalize(T - dot(T, N) * N);
        B = cross(N, T) * vertexTangent.w;
    }
    else
    {
        // Simple fallback TBN
        vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
        T = normalize(cross(up, N));
        B = cross(N, T);
    }
    
    fragNormal = N;
    fragTBN = mat3(T, B, N);

    gl_Position = mvp * vec4(vPos, 1.0);
}
