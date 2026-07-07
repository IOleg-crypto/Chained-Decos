#ifndef CH_RENDERER_DATA_H
#define CH_RENDERER_DATA_H

#include "engine/common/timestep.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/graphics/pipeline/shader_storage.h"
#include "engine/graphics/api/storage_buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include <glm/glm.hpp>
#include <memory>

namespace Chained
{
    struct RenderLight
    {
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes
        glm::vec3 position = {0, 0, 0};              // 12 bytes
        float intensity = 1.0f;                  // 4 bytes
        glm::vec3 direction = {0, -1, 0};            // 12 bytes
        float radius = 10.0f;                     // 4 bytes
        float innerCutoff = 15.0f;                // 4 bytes
        float outerCutoff = 20.0f;                // 4 bytes
        int type = 0;                               // 4 bytes
        int enabled = 0;                            // 4 bytes
    };

    struct CameraData
    {
        glm::mat4 ViewProjection;
        glm::mat4 Projection;
        glm::mat4 View;
    };

    struct RendererData
    {
        std::unique_ptr<ShaderStorage> Shaders;

        std::shared_ptr<UniformBuffer> CameraUBO;
        std::shared_ptr<UniformBuffer> GlobalUBO;

        float DiagnosticMode = 0.0f;
        glm::vec3 CurrentCameraPosition = {0.0f, 0.0f, 0.0f};
        Timestep Time = 0.0f;
        int LightCount = 0;
        unsigned int CurrentShaderId = 0;
        EnvironmentSettings CurrentEnv;

        glm::mat4 CurrentView = glm::mat4(1.0f);
        glm::mat4 CurrentProj = glm::mat4(1.0f);

        std::shared_ptr<VertexArray> FullscreenQuadVAO;
        std::shared_ptr<VertexArray> BillboardVAO;
        std::shared_ptr<VertexArray> SpriteVAO;
        std::shared_ptr<VertexArray> GridPlaneVAO;
    };
}

#endif // CH_RENDERER_DATA_H
