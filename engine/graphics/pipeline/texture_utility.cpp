#include <glm/gtc/type_ptr.hpp>
#include "engine/graphics/pipeline/texture_utility.h"
#include "engine/graphics/api/texture.h"
#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/assets/model_asset.h"
#include <glad/gl.h>

namespace CHEngine
{
    std::shared_ptr<Texture> TextureUtility::GenTextureCubemap(uint32_t shaderId, uint32_t panoramaId, int size, const Mesh& cubeMesh)
    {
        auto cubemap = Texture::CreateCubemap(size, TextureFormat::RGBA16F);
        
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        
        auto spec = FramebufferSpecification();
        spec.Width = size;
        spec.Height = size;
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

        // TODO: Shader abstraction needed here too for cleaner code
        glUseProgram(shaderId);
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, panoramaId);
        glUniform1i(glGetUniformLocation(shaderId, "equirectangularMap"), 0);

        captureFBO->Bind();
        RenderCommand::SetViewport(0, 0, size, size);
        RenderCommand::SetCullMode(RendererAPI::CullMode::None);
        RenderCommand::DisableDepthTest();

        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(glGetUniformLocation(shaderId, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            
            // Still need a way to attach cubemap face to FBO in abstraction
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap->GetRendererID(), 0);
            
            RenderCommand::Clear({0, 0, 0, 255});
            
            cubeMesh.VAO->Bind();
            RenderCommand::DrawIndexed(cubeMesh.VAO, 36);
        }
        
        captureFBO->Unbind();
        RenderCommand::SetViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        
        return cubemap;
    }
}
