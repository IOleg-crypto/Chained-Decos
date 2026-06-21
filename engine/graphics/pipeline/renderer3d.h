#ifndef CH_RENDERER3D_H
#define CH_RENDERER3D_H

#include "engine/foundation/base.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/camera_types.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Chained {

    struct Renderer3DData
    {
        std::shared_ptr<VertexBuffer> InstanceBuffer;
        uint32_t InstanceBufferCapacity = 0;
        std::unordered_map<VertexArray*, std::shared_ptr<VertexArray>> InstancedVAOCache;

        std::unique_ptr<Model> UnitCubeModel;
        std::unique_ptr<Model> UnitSphereModel;
        std::unique_ptr<Model> UnitCapsuleModel;
        std::unique_ptr<Model> WireCubeModel;

        std::unique_ptr<Model> SkyboxCubeModel;
        std::unique_ptr<Model> SkyboxSphereModel;
    };

    static Renderer3DData s_3DData;

    class CH_API Renderer3D {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const Camera3D& camera, float nearClip = 0.01f, float farClip = 10000.0f);
        static void EndScene();

        static void DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);
        static void DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>& transforms);
        static void DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform, bool useWireframe = true);
        
        static void DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness, float contrast, const Camera3D& camera, bool flipped = false);
        
        static void DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color, bool useWireframe = true);
        static void DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color, bool useWireframe = true);
        static void DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color, bool useWireframe = true);
    };
}

#endif // CH_RENDERER3D_H
