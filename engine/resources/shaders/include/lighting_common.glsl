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
    vec4 color;
    vec3 position;
    vec3 direction;
    float intensity;
    float radius;
    float innerCutoff;
    float outerCutoff;
    int type;       // 0: Point, 1: Spot
    int enabled;
};

#define MAX_LIGHTS 16
uniform Light lights[MAX_LIGHTS];
