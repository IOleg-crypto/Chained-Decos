// Debug Visualization Modes


const int DEBUG_MODE_DISABLED = 0;
const int DEBUG_MODE_NORMALS = 1;
const int DEBUG_MODE_LIGHTING_ONLY = 2;
const int DEBUG_MODE_UNLIT_ALBEDO = 3;
const int DEBUG_MODE_METALNESS = 4;
const int DEBUG_MODE_ROUGHNESS = 5;
const int DEBUG_MODE_OCCLUSION = 6;


vec4 ApplyDebugMode(in int mode,
                    in vec3 normal,
                    in vec3 linearColor,
                    in float metalness,
                    in float roughness,
                    in float occlusion,
                    in float baseAlpha)
{
    switch(mode)
    {
        case DEBUG_MODE_NORMALS:
            
            return vec4(normal * 0.5 + 0.5, baseAlpha);
            
        case DEBUG_MODE_LIGHTING_ONLY:
            
            return vec4(0.5, 0.5, 0.5, 1.0);
            
        case DEBUG_MODE_UNLIT_ALBEDO:
            
            return vec4(linearColor, baseAlpha);
            
        case DEBUG_MODE_METALNESS:
            
            return vec4(vec3(metalness), baseAlpha);
            
        case DEBUG_MODE_ROUGHNESS:
            
            return vec4(vec3(roughness), baseAlpha);
            
        case DEBUG_MODE_OCCLUSION:
            
            return vec4(vec3(occlusion), baseAlpha);
            
        default:
            
            return vec4(linearColor, baseAlpha);
    }
}
