#ifndef CH_API_CONTEXT_H
#define CH_API_CONTEXT_H

#include "engine/render/environment.h"
#include "render_types.h"

namespace CHEngine
{
class APIContext
{
public:
    static void Init();
    static void Shutdown();

    static RendererState &GetState()
    {
        return s_State;
    }

    static void SetDirectionalLight(Vector3 direction, Color color);
    static void SetAmbientLight(float intensity);
    static void ApplyEnvironment(const EnvironmentSettings &settings);

private:
    static RendererState s_State;
};
} // namespace CHEngine

#endif // CH_API_CONTEXT_H
