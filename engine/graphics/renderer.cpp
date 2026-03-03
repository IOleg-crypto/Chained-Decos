#include "engine/graphics/renderer.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/shader_asset.h"
#include "rlgl.h"
#include <algorithm>
#include <vector>

namespace CHEngine
{

Renderer* Renderer::s_Instance = nullptr;

bool Renderer::IsInitialized()
{
    return s_Instance != nullptr && s_Instance->m_Data != nullptr;
}

Renderer& Renderer::Get()
{
    CH_CORE_ASSERT(s_Instance, "Renderer is not initialized!");
    return *s_Instance;
}

void Renderer::Init()
{
    if (s_Instance)
    {
        CH_CORE_WARN("Renderer is already initialized!");
        return;
    }

    s_Instance = new Renderer();
    CH_CORE_INFO("Render System Initialized (Core).");
}

void Renderer::LoadEngineResources(AssetManager& assetManager)
{
    CH_CORE_INFO("Renderer: Loading engine materials and shaders...");
    auto& renderer = Renderer::Get();
    auto& lib = renderer.GetShaderLibrary();

    auto loadShader = [&](const std::string& name, const std::string& path) {
        auto shader = assetManager.Get<ShaderAsset>(path);
        if (shader)
        {
            lib.Add(name, shader);
            return true;
        }
        return false;
    };

    loadShader("Lighting", "engine/resources/shaders/lighting.chshader");
    loadShader("Skybox", "engine/resources/shaders/skybox.chshader");
    loadShader("Unlit", "engine/resources/shaders/unlit.chshader");
    loadShader("CubemapGen", "engine/resources/shaders/cubemap.chshader");
    loadShader("SkyboxCubemap", "engine/resources/shaders/skybox_cubemap.chshader");
    loadShader("PostProcess", "engine/resources/shaders/post_process.chshader");
}

void Renderer::Shutdown()
{
    if (!s_Instance)
    {
        return;
    }

    CH_CORE_INFO("Shutting down Render System...");

    Renderer* instance = s_Instance;
    s_Instance = nullptr;
    delete instance;
}

Renderer::Renderer()
{
    m_Data = std::make_unique<RendererData>();
    m_Data->Shaders = std::make_unique<ShaderLibrary>();
}

Renderer::~Renderer()
{
}

Texture2D Renderer::GetOrLoadTexture(const std::string& path)
{
    if (path.empty())
    {
        return {0};
    }

    if (m_Data->TextureCache.count(path))
    {
        return m_Data->TextureCache[path];
    }

    if (FileExists(path.c_str()))
    {
        CH_CORE_INFO("Renderer: Loading texture '{}' into GPU cache...", path);

        Image image = LoadImage(path.c_str());
        if (image.data)
        {
            // HDR Tone mapping
            bool isHDR = (image.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32 ||
                          image.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
            if (isHDR)
            {
                int channels = (image.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32) ? 3 : 4;
                int pixelCount = image.width * image.height;
                float* pixels = (float*)image.data;
                for (int i = 0; i < pixelCount; i++)
                {
                    for (int c = 0; c < 3; c++)
                    {
                        float v = pixels[i * channels + c];
                        float mapped = (v * (2.51f * v + 0.03f)) / (v * (2.43f * v + 0.59f) + 0.14f);
                        if (mapped < 0.0f)
                        {
                            mapped = 0.0f;
                        }
                        if (mapped > 1.0f)
                        {
                            mapped = 1.0f;
                        }
                        mapped = powf(mapped, 1.0f / 2.2f);
                        pixels[i * channels + c] = mapped;
                    }
                }
                ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            }

            Texture2D texture = LoadTextureFromImage(image);
            UnloadImage(image);

            if (texture.id > 0)
            {
                m_Data->TextureCache[path] = texture;
                return texture;
            }
        }
    }

    return {0};
}

void Renderer::BeginScene(const Camera3D& camera)
{
    BeginMode3D(camera);
}

void Renderer::EndScene()
{
    EndMode3D();
}

void Renderer::Clear(Color color)
{
    ClearBackground(color);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    rlViewport(x, y, width, height);
}

void Renderer::DrawLine(Vector3 startPosition, Vector3 endPosition, Color color)
{
    DrawLine3D(startPosition, endPosition, color);
}

void Renderer::DrawGrid(int sliceCount, float spacing)
{
    ::DrawGrid(sliceCount, spacing);
}

void Renderer::DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color color)
{
    if (texture.id == 0)
    {
        return;
    }
    ::DrawBillboard(camera, texture, position, size, color);
}

void Renderer::DrawCubeWires(const Matrix& transform, Vector3 size, Color color)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    ::DrawCubeWires({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);
    rlPopMatrix();
}

void Renderer::DrawCapsuleWires(const Matrix& transform, float radius, float height, Color color)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));

    float cylinderHeight = height - 2.0f * radius;
    if (cylinderHeight < 0)
    {
        cylinderHeight = 0;
    }
    float halfCylinder = cylinderHeight * 0.5f;

    if (cylinderHeight > 0)
    {
        Vector3 start = {0, -halfCylinder, 0};
        Vector3 end = {0, halfCylinder, 0};
        ::DrawCylinderWiresEx(start, end, radius, radius, 8, color);
    }

    ::DrawSphereWires({0, -halfCylinder, 0}, radius, 8, 8, color);
    ::DrawSphereWires({0, halfCylinder, 0}, radius, 8, 8, color);

    rlPopMatrix();
}

void Renderer::DrawSphereWires(const Matrix& transform, float radius, Color color)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    ::DrawSphereWires({0, 0, 0}, radius, 8, 8, color);
    rlPopMatrix();
}

void Renderer::ApplyPostProcessing(RenderTexture2D target, const Camera3D& camera)
{
    CH_PROFILE_FUNCTION();

    auto shaderAsset = m_Data->Shaders->Exists("PostProcess") ? m_Data->Shaders->Get("PostProcess") : nullptr;
    if (!shaderAsset)
    {
        return;
    }

    Matrix view = GetCameraMatrix(camera);
    Matrix proj = rlGetMatrixProjection();
    Matrix viewProj = MatrixMultiply(view, proj);
    Matrix invViewProj = MatrixInvert(viewProj);

    ShaderAsset* shader = shaderAsset.get();
    rlEnableShader(shader->GetShader().id);

    shader->SetMatrix("matInverseViewProj", invViewProj);
    shader->SetVec3("viewPos", camera.position);
    shader->SetFloat("uTime", (float)GetTime());

    shader->SetInt("texture1", 1);
    rlActiveTextureSlot(1);
    rlEnableTexture(target.depth.id);
    rlActiveTextureSlot(0);

    Rectangle source = {0, 0, (float)target.texture.width, (float)-target.texture.height};
    Rectangle dest = {0, 0, (float)target.texture.width, (float)target.texture.height};
    ::DrawTexturePro(target.texture, source, dest, {0, 0}, 0.0f, WHITE);

    rlActiveTextureSlot(1);
    rlDisableTexture();
    rlActiveTextureSlot(0);
    rlDisableShader();
}

void Renderer::SetDiagnosticMode(float mode)
{
    m_Data->DiagnosticMode = mode;
}

void Renderer::UpdateTime(Timestep time)
{
    m_Data->Time = time;
}

} // namespace CHEngine
