#include <glm/gtc/type_ptr.hpp>
#include "engine/graphics/pipeline/texture_utility.h"
#include "engine/graphics/api/texture.h"
#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/api/shader.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/assets/types/model_asset.h"
#include <glad/gl.h>

namespace Chained
{
    void TextureUtility::FlipImageVertically(void* data, int width, int height, int channels, bool isHDR)
    {
        if (!data || width <= 0 || height <= 0 || channels <= 0) return;

        size_t pixelSize = isHDR ? sizeof(float) : sizeof(unsigned char);
        size_t rowSize = (size_t)width * channels * pixelSize;
        uint8_t* pData = (uint8_t*)data;
        std::vector<uint8_t> rowBuffer(rowSize);

        for (int i = 0; i < height / 2; ++i)
        {
            uint8_t* row1 = pData + (size_t)i * rowSize;
            uint8_t* row2 = pData + (size_t)(height - 1 - i) * rowSize;
            std::memcpy(rowBuffer.data(), row1, rowSize);
            std::memcpy(row1, row2, rowSize);
            std::memcpy(row2, rowBuffer.data(), rowSize);
        }
    }

    std::shared_ptr<Texture> TextureUtility::GenTextureCubemap(const std::shared_ptr<Shader>& shader, uint32_t panoramaId, int size, const Mesh& cubeMesh)
    {
        auto cubemap = Texture::CreateCubemap(size, TextureFormat::RGBA16F);
        
        int viewport[4];
        GraphicsDevice::Get().GetViewport(&viewport[0], &viewport[1], &viewport[2], &viewport[3]);
        
        auto spec = FramebufferSpecification();
        spec.Width = size;
        spec.Height = size;
        spec.ColorFormat = FramebufferColorFormat::RGBA16F;
        auto captureFBO = Framebuffer::Create(spec);
        
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        shader->Bind();
        shader->SetMatrix("projection", captureProjection);
        
        GraphicsDevice::Get().SetTexture(0, panoramaId);
        shader->SetInt("equirectangularMap", 0);

        captureFBO->Bind();
        GraphicsDevice::Get().SetViewport(0, 0, size, size);
        GraphicsDevice::Get().SetCullMode(GraphicsDevice::CullMode::None);
        GraphicsDevice::Get().DisableDepthTest();

        for (unsigned int i = 0; i < 6; ++i)
        {
            shader->SetMatrix("view", captureViews[i]);
            
            // Still need a way to attach cubemap face to FBO in abstraction
            // For now, keep the raw GL call for the face attachment as it's very specific to this utility
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap->GetNativeHandle(), 0);
            
            GraphicsDevice::Get().Clear({0, 0, 0, 255});
            
            cubeMesh.VAO->Bind();
            GraphicsDevice::Get().DrawIndexed(cubeMesh.VAO, 36);
        }
        
        captureFBO->Unbind();
        GraphicsDevice::Get().SetViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        
        return cubemap;
    }
}
