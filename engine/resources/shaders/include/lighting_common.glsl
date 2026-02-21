// Common Light data
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform float ambient;
uniform vec3 viewPos;
uniform float uTime;

// Diagnostics
uniform float uMode; // 0: Normal, 1: Normals Visualized, 2: Lighting only, 3: Albedo only

// Material properties
uniform sampler2D texture0; // Albedo
uniform sampler2D texture1; // Metallic
uniform sampler2D texture2; // Normal
uniform sampler2D texture3; // Roughness
uniform sampler2D texture4; // Occlusion
uniform sampler2D texture5; // Emission

uniform vec4 colDiffuse;
uniform vec4 colEmissive;

uniform int useTexture;
uniform int useNormalMap;
uniform int useMetallicMap;
uniform int useRoughnessMap;
uniform int useOcclusionMap;
uniform int useEmissiveTexture;

uniform float metalness;
uniform float roughness;
uniform float shininess; // Legacy support (derived from roughness)
uniform float emissiveIntensity;

struct Light {
    vec4 color;       // 0-15
    vec3 position;    // 16-27
    float intensity;  // 28-31
    vec3 direction;   // 32-43
    float radius;     // 44-47
    float innerCutoff;// 48-51
    float outerCutoff;// 52-55
    int type;         // 56-59
    int enabled;      // 60-63
};

#define MAX_LIGHTS 256
layout(std430, binding = 0) buffer LightData {
    Light lights[];
};
