#include "engine/graphics/pipeline/frame_manager.h"
#include "engine/graphics/camera_types.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{

FrameManager::FrameManager(FrameState& frame, std::shared_ptr<UniformBuffer> cameraUBO)
    : m_Frame(frame), m_CameraUBO(std::move(cameraUBO))
{
}

void FrameManager::BeginScene(const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                              float nearClip, float farClip)
{
    m_Frame.CameraPosition = camera.Position;

    // View matrix
    m_Frame.View = glm::lookAt(camera.Position, camera.Target, camera.Up);

    // Projection matrix
    float aspect = (viewportHeight > 0) ? static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight) : 1.0f;
    if (camera.Projection == ProjectionType::Perspective)
    {
        m_Frame.Proj = glm::perspective(glm::radians(camera.FovDegrees), aspect, nearClip, farClip);
    }
    else
    {
        float top = camera.FovDegrees / 2.0f;
        float right = top * aspect;
        m_Frame.Proj = glm::ortho(-right, right, -top, top, nearClip, farClip);
    }

    // Upload to UBO
    CameraData cameraData;
    cameraData.ViewProjection = m_Frame.Proj * m_Frame.View;
    cameraData.Projection = m_Frame.Proj;
    cameraData.View = m_Frame.View;
    m_CameraUBO->SetData(&cameraData, sizeof(CameraData));
    m_CameraUBO->BindBase(0);
}

void FrameManager::EndScene()
{
    m_Frame.CurrentShaderId = 0;
}

void FrameManager::SetTime(float time)
{
    m_Frame.Time = time;
}

void FrameManager::SetDiagnosticMode(float mode)
{
    m_Frame.DiagnosticMode = mode;
}

} // namespace Chained
