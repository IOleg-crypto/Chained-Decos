#ifndef CH_DEBUG_RENDERER_H
#define CH_DEBUG_RENDERER_H

#include "engine/graphics/camera_types.h"
#include "engine/graphics/api/renderer_types.h"
#include "engine/scene/components/model_component.h"
#include "engine/core/engine_module.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Chained
{

struct LineVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
};

struct LineState
{
    std::shared_ptr<class VertexBuffer> VBO;
    std::shared_ptr<VertexArray>        VAO;
    std::vector<LineVertex>             Vertices;
    uint32_t                            VBOSize = 0;
};

struct StaticResources
{
    std::unique_ptr<Model> UnitCubeModel;
    std::unique_ptr<Model> UnitSphereModel;
    std::unique_ptr<Model> UnitCapsuleModel;
    std::unique_ptr<Model> WireCubeModel;
};

class CH_API DebugRendererService : public EngineModule
{
public:
    DebugRendererService() = default;
    virtual ~DebugRendererService() override = default;

    LineState        Lines;
    StaticResources  Resources;
    std::shared_ptr<VertexArray> GridPlaneVAO;

protected:
    virtual void Initialize() override;
    virtual void Shutdown() override;
};

namespace DebugRenderer
{
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
    void Flush();
    void DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe = true);
    void DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool useWireframe = true);
    void DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color, bool useWireframe = true);
    void DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe = true);
    void DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color);
} // namespace DebugRenderer

} // namespace Chained

#endif // CH_DEBUG_RENDERER_H
