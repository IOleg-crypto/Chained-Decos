#include "EditorGrid.h"
#include <raymath.h>
#include <rlgl.h>

EditorGrid::EditorGrid()
{
    m_Shader = {0};
}

EditorGrid::~EditorGrid()
{
    if (m_Shader.id != 0)
    {
        UnloadShader(m_Shader);
    }
}

void EditorGrid::Init()
{
    const char *vsPath = TextFormat("%s/resources/shaders/infinite_grid.vs", PROJECT_ROOT_DIR);
    const char *fsPath = TextFormat("%s/resources/shaders/infinite_grid.fs", PROJECT_ROOT_DIR);

    m_Shader = LoadShader(vsPath, fsPath);

    if (m_Shader.id == 0)
    {
        TraceLog(LOG_ERROR, "SHADER: [ID 0] Failed to load infinite grid shader");
    }
    else
    {
        m_ViewLoc = GetShaderLocation(m_Shader, "matView");
        m_ProjLoc = GetShaderLocation(m_Shader, "matProjection");
        m_NearLoc = GetShaderLocation(m_Shader, "near");
        m_FarLoc = GetShaderLocation(m_Shader, "far");
        TraceLog(LOG_INFO, "SHADER: [ID %d] Infinite grid shader loaded successfully", m_Shader.id);
    }
}

void EditorGrid::Draw(Camera3D camera, int width, int height)
{
    if (m_Shader.id == 0)
        return;

    Matrix view = GetCameraMatrix(camera);
    float aspect = (float)width / (float)height;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, nearPlane, farPlane);

    // Flush batch before state changes
    rlDrawRenderBatchActive();

    BeginShaderMode(m_Shader);
    SetShaderValueMatrix(m_Shader, m_ViewLoc, view);
    SetShaderValueMatrix(m_Shader, m_ProjLoc, projection);
    SetShaderValue(m_Shader, m_NearLoc, &nearPlane, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_Shader, m_FarLoc, &farPlane, SHADER_UNIFORM_FLOAT);

    // Reset matrices to draw in clip space
    Matrix oldProj = rlGetMatrixProjection();
    Matrix oldView = rlGetMatrixModelview();

    // We want to KEEP depth testing enabled so the grid is depth-tested against objects
    rlDisableBackfaceCulling();
    rlEnableDepthTest();
    rlSetMatrixProjection(MatrixIdentity());
    rlSetMatrixModelview(MatrixIdentity());

    // Draw a full-screen quad at the far plane (Z=1.0 in NDC)
    rlBegin(RL_TRIANGLES);
    rlVertex3f(-1.0f, -1.0f, 1.0f);
    rlVertex3f(1.0f, -1.0f, 1.0f);
    rlVertex3f(1.0f, 1.0f, 1.0f);

    rlVertex3f(1.0f, 1.0f, 1.0f);
    rlVertex3f(-1.0f, 1.0f, 1.0f);
    rlVertex3f(-1.0f, -1.0f, 1.0f);
    rlEnd();

    rlDrawRenderBatchActive();

    // Restore state
    rlSetMatrixProjection(oldProj);
    rlSetMatrixModelview(oldView);
    rlEnableBackfaceCulling();

    EndShaderMode();
}
