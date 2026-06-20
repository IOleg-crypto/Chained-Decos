#ifndef CH_RENDERER2D_H
#define CH_RENDERER2D_H

#include "engine/foundation/base.h"
#include "engine/scene/camera_types.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Chained {
    class CH_API Renderer2D {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const Camera3D& camera);
        static void EndScene();

        static void DrawSprite(uint32_t textureId, const glm::mat4& transform, const glm::vec4& tint, bool flipX = false, bool flipY = false);
        static void DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size, const glm::vec4& tint);
        
        // Line rendering
        static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
    };
}

#endif // CH_RENDERER2D_H
