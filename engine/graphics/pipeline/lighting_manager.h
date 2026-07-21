#ifndef CH_LIGHTING_MANAGER_H
#define CH_LIGHTING_MANAGER_H

#include "engine/assets/types/environment_asset.h"
#include "engine/graphics/api/renderer_types.h"
#include "engine/graphics/pipeline/renderer_data.h"

namespace Chained
{

class ShaderAsset;

/// @brief Manages per-frame lighting state: lights, SSBO upload, fog, shadows.
/// Operates on LightingData owned by Renderer — no separate storage.
class CH_API LightingManager
{
public:
    explicit LightingManager(LightingData& lighting, ShadowState& shadow);

    // --- Light management ---
    void SetLight(int index, const RenderLight& light);
    void SetLightCount(int count);
    void ClearLights();

    // --- Environment / main light ---
    void ApplyEnvironment(const EnvironmentSettings& settings);
    void SetMainLight(const LightingSettings& settings);

    // --- Frame upload ---
    /// Upload dirty light SSBO to GPU. Call once per frame in BeginScene.
    void UploadLights();
    /// Set shadow map data from ShadowPass output.
    void SetShadowState(bool enabled, uint32_t mapTextureID, const glm::mat4& lightSpaceMatrix, float bias);

    // --- Shader uniform binding ---
    void SetLightingUniforms(ShaderAsset* shaderAsset, const FrameState& frame);
    /// Applies fog uniforms to the given shader asset.
    void ApplyFogUniforms(ShaderAsset* shader);

    // --- Accessors for direct data access ---
    LightingData&       GetData()       { return m_Lighting; }
    const LightingData& GetData() const { return m_Lighting; }
    ShadowState&        GetShadowData()       { return m_Shadow; }
    const ShadowState&  GetShadowData() const { return m_Shadow; }

private:
    LightingData& m_Lighting;
    ShadowState&  m_Shadow;
};

} // namespace Chained

#endif // CH_LIGHTING_MANAGER_H
