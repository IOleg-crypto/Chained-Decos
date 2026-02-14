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
uniform sampler2D texture1; // Emissive
uniform vec4 colDiffuse;
uniform vec4 colEmissive;
uniform int useTexture;
uniform int useEmissiveTexture;
uniform float shininess;
uniform float emissiveIntensity;

struct PointLight {
    vec4 color;
    vec3 position;
    float intensity;
    float radius;
    int enabled;
};

struct SpotLight {
    vec4 color;
    vec3 position;
    vec3 direction;
    float intensity;
    float range;
    float innerCutoff;
    float outerCutoff;
    int enabled;
};

#define MAX_LIGHTS 8
uniform PointLight pointLights[MAX_LIGHTS];
uniform SpotLight spotLights[MAX_LIGHTS];
