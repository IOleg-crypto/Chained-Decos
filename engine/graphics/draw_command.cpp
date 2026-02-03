#include "engine/graphics/draw_command.h"
#include "engine/core/profiler.h"
#include "engine/graphics/api_context.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/texture_asset.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "raymath.h"
#include "rlgl.h"
#include <filesystem>

namespace CHEngine
{

    RendererState &DrawCommand::GetState()
    {
        return APIContext::GetState();
    }

    void DrawCommand::Init()
    {
        // High-level init now handled by Visuals/APIContext
    }

    void DrawCommand::Shutdown()
    {
        // High-level shutdown now handled by Visuals/APIContext
    }

    void DrawCommand::SetDirectionalLight(Vector3 direction, Color color)
    {
        APIContext::SetDirectionalLight(direction, color);
    }

    void DrawCommand::SetAmbientLight(float intensity)
    {
        APIContext::SetAmbientLight(intensity);
    }

    void DrawCommand::ApplyEnvironment(const EnvironmentSettings &settings)
    {
        APIContext::ApplyEnvironment(settings);
    }
    void DrawCommand::Clear(Color color)
    {
        ::ClearBackground(color);
    }

    void DrawCommand::SetViewport(int x, int y, int width, int height)
    {
        rlViewport(x, y, width, height);
    }

    
    static void ApplyMaterialOverrides(Material &mat, int meshIndex, int matIndex,
                                       const std::vector<MaterialSlot> &overrides,
                                       RenderExtraState &extra)
    {
        for (const auto &slot : overrides)
        {
            bool matches = (slot.Index == -1);
            if (slot.Target == MaterialSlotTarget::MaterialIndex)
                matches |= (slot.Index == matIndex);
            else if (slot.Target == MaterialSlotTarget::MeshIndex)
                matches |= (slot.Index == meshIndex);

            if (matches)
            {
                const auto &material = slot.Material;
                
                // Custom Graphics State
                extra.DoubleSided = material.DoubleSided;
                extra.Transparent = material.Transparent;
                extra.Alpha = material.Alpha;

                // Apply custom shader if provided
                if (material.OverrideShader && !material.ShaderPath.empty())
                {
                    auto shaderAsset = AssetManager::Get<ShaderAsset>(material.ShaderPath);
                    if (shaderAsset)
                        mat.shader = shaderAsset->GetShader();
                }

                if (material.OverrideAlbedo)
                    mat.maps[MATERIAL_MAP_ALBEDO].color = material.AlbedoColor;

                // Apply alpha to albedo color
                mat.maps[MATERIAL_MAP_ALBEDO].color.a = (unsigned char)(mat.maps[MATERIAL_MAP_ALBEDO].color.a * extra.Alpha);

                if (!material.AlbedoPath.empty())
                {
                    auto texAsset = AssetManager::Get<TextureAsset>(material.AlbedoPath);
                    if (texAsset)
                        mat.maps[MATERIAL_MAP_ALBEDO].texture = texAsset->GetTexture();
                }

                if (material.OverrideNormal && !material.NormalMapPath.empty())
                {
                    auto texAsset = AssetManager::Get<TextureAsset>(material.NormalMapPath);
                    if (texAsset)
                        mat.maps[MATERIAL_MAP_NORMAL].texture = texAsset->GetTexture();
                }
            }
        }
    }

    void DrawCommand::DrawModel(const std::string &path, const Matrix &transform,
                                const std::vector<MaterialSlot> &overrides,
                                int animIndex, int frame)
    {
        auto asset = AssetManager::Get<ModelAsset>(path);
        if (!asset || asset->GetState() != AssetState::Ready)
            return;

        Model &model = asset->GetModel();
        if (model.meshCount == 0) return;

        // Apply animation to CPU mesh data if requested
        if (animIndex != -1)
        {
            asset->UpdateAnimation(animIndex, frame);
        }

        Matrix finalTransform = MatrixMultiply(model.transform, transform);
        auto &state = APIContext::GetState();

        

        // Update Profiler
        ProfilerStats stats;
        stats.DrawCalls++;
        stats.MeshCount += model.meshCount;
        for (int i = 0; i < model.meshCount; i++)
            stats.PolyCount += model.meshes[i].triangleCount;
        Profiler::UpdateStats(stats);

        for (int i = 0; i < model.meshCount; i++)
        {
            int matIndex = model.meshMaterial[i];
            Material mat = model.materials[matIndex]; // Copy of asset material

            // Use custom lighting shader ONLY for file-based models that don't have one
            if (mat.shader.id == 0 && state.LightingShader && !path.starts_with(":"))
                mat.shader = state.LightingShader->GetShader();

            // Apply overrides to our local material instance
            RenderExtraState extra;
            ApplyMaterialOverrides(mat, i, matIndex, overrides, extra);

            // Adjust alpha in the actual material color
            mat.maps[MATERIAL_MAP_ALBEDO].color.a = (unsigned char)(mat.maps[MATERIAL_MAP_ALBEDO].color.a * extra.Alpha);

            // Perform the draw
            ::DrawMesh(model.meshes[i], mat, finalTransform);

        }
    }

    void DrawCommand::DrawLine(Vector3 start, Vector3 end, Color color)
    {
        ::DrawLine3D(start, end, color);
    }

    void DrawCommand::DrawGrid(int slices, float spacing)
    {
        ::DrawGrid(slices, spacing);
    }

    void DrawCommand::DrawSkybox(const SkyboxSettings &skybox, const Camera3D &camera)
    {
        if (skybox.TexturePath.empty())
        {
            CH_CORE_WARN("DrawSkybox: TexturePath is empty");
            return;
        }

        auto texAsset = AssetManager::Get<TextureAsset>(skybox.TexturePath);
        if (!texAsset || texAsset->GetState() != AssetState::Ready)
        {
            CH_CORE_WARN("DrawSkybox: Texture '{}' not loaded (state={})", 
                skybox.TexturePath, texAsset ? (int)texAsset->GetState() : -1);
            return;
        }

        auto &state = APIContext::GetState();
        
        // Validate skybox cube is initialized
        if (state.SkyboxCube.meshCount == 0)
        {
            CH_CORE_WARN("DrawSkybox: SkyboxCube mesh not initialized");
            return;
        }
            
        bool usePanorama = std::filesystem::path(skybox.TexturePath).extension() != ".hdr";

        std::shared_ptr<ShaderAsset> shaderAsset = usePanorama ? state.PanoramaShader : state.SkyboxShader;
        if (!shaderAsset || shaderAsset->GetState() != AssetState::Ready)
        {
            CH_CORE_WARN("DrawSkybox: {} shader not loaded", usePanorama ? "Panorama" : "Skybox");
            return;
        }

        auto &shader = shaderAsset->GetShader();
        
        // Validate shader is loaded
        if (shader.id == 0)
        {
            CH_CORE_WARN("DrawSkybox: Shader ID is 0");
            return;
        }
            
        state.SkyboxCube.materials[0].shader = shader;
        SetMaterialTexture(&state.SkyboxCube.materials[0], usePanorama ? MATERIAL_MAP_ALBEDO : MATERIAL_MAP_CUBEMAP,
                           texAsset->GetTexture());

        // Set Uniforms
        if (usePanorama)
        {
            shaderAsset->SetInt("doGamma", 1);
            shaderAsset->SetFloat("fragGamma", 2.2f);
            shaderAsset->SetFloat("exposure", skybox.Exposure);
            shaderAsset->SetFloat("brightness", skybox.Brightness);
            shaderAsset->SetFloat("contrast", skybox.Contrast);
        }
        else
        {
            shaderAsset->SetInt("vflipped", 0);
            shaderAsset->SetInt("doGamma", 1);
            shaderAsset->SetFloat("fragGamma", 2.2f);
            shaderAsset->SetFloat("exposure", skybox.Exposure);
            shaderAsset->SetFloat("brightness", skybox.Brightness);
            shaderAsset->SetFloat("contrast", skybox.Contrast);
        }

        rlDisableBackfaceCulling();
        rlDisableDepthMask();
        ::DrawModel(state.SkyboxCube, camera.position, 1.0f, WHITE);
        rlEnableDepthMask();
        rlEnableBackfaceCulling();
    }

    void DrawCommand::DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length,
                                      Color color)
    {
        float x = position.x, y = position.y, z = position.z;
        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);

        // Front Face
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

        // Back Face
        rlNormal3f(0.0f, 0.0f, -1.0f);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2, z - length / 2);

        // Top Face
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2, z - length / 2);

        // Bottom Face
        rlNormal3f(0.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2, z + length / 2);

        // Right Face
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2, z + length / 2);

        // Left Face
        rlNormal3f(-1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2, z - length / 2);

        rlEnd();
        rlSetTexture(0);
    }
} // namespace CHEngine
