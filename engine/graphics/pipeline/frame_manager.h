#ifndef CH_FRAME_MANAGER_H
#define CH_FRAME_MANAGER_H

#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/renderer_types.h"
#include "engine/graphics/pipeline/renderer_data.h"

namespace Chained
{

/// @brief Manages per-frame GPU state: camera UBO, view/projection matrices, time, diagnostics.
/// Operates on FrameState owned by Renderer — no separate storage.
class CH_API FrameManager
{
public:
    explicit FrameManager(FrameState& frame, std::shared_ptr<UniformBuffer> cameraUBO);

    /// Upload camera matrices to UBO and compute view/projection for the given viewport.
    void BeginScene(const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                    float nearClip, float farClip);

    /// Reset transient frame state.
    void EndScene();

    void SetTime(float time);
    void SetDiagnosticMode(float mode);

    // --- Accessors ---
    const glm::mat4&  GetView()        const { return m_Frame.View; }
    const glm::mat4&  GetProj()        const { return m_Frame.Proj; }
    glm::vec3         GetCameraPos()   const { return m_Frame.CameraPosition; }
    float             GetTime()        const { return m_Frame.Time; }
    float             GetDiagnosticMode() const { return m_Frame.DiagnosticMode; }
    unsigned int      GetCurrentShaderId() const { return m_Frame.CurrentShaderId; }
    void SetCurrentShaderId(unsigned int id) { m_Frame.CurrentShaderId = id; }

    FrameState&       GetData()       { return m_Frame; }
    const FrameState& GetData() const { return m_Frame; }

private:
    FrameState& m_Frame;
    std::shared_ptr<UniformBuffer> m_CameraUBO;
};

} // namespace Chained

#endif // CH_FRAME_MANAGER_H
