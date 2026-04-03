#include "render_command.h"
#include <glad/gl.h>

namespace CHEngine
{

std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

void RenderCommand::Initialize()
{
    s_RendererAPI->Init();
}

void RenderCommand::Shutdown()
{
}
    // These empty stubs were removed in favor of the full implementation in Renderer::DrawLine/DrawGrid

} // namespace CHEngine
