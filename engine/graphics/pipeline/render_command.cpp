#include "render_command.h"
#include <glad/gl.h>

namespace Chained
{

std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

void RenderCommand::Initialize()
{
    s_RendererAPI->Init();
}

void RenderCommand::Shutdown()
{
    s_RendererAPI.reset();
}


} // namespace Chained
