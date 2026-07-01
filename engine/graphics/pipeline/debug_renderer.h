#ifndef CH_DEBUG_RENDERER_H
#define CH_DEBUG_RENDERER_H

#include "engine/graphics/camera_types.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/scene/components/mesh_component.h"
#include <glm/glm.hpp>
#include <vector>

namespace Chained
{
    namespace DebugRenderer
    {
        void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
        void DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe = true);
        void DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool useWireframe = true);
        void DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color, bool useWireframe = true);
        void DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe = true);
        void DrawGrid(int slices, float spacing);
        void DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color);
    };
} // namespace Chained

#endif // CH_DEBUG_RENDERER_H
