layout (std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
    mat4 u_Projection;
    mat4 u_View;
};
