#ifndef CH_API_CONTEXT_H
#define CH_API_CONTEXT_H

#include "engine/graphics/environment.h"
#include "engine/graphics/render_types.h"


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

    static void ApplyEnvironment(const EnvironmentSettings &settings);
    static void SetDirectionalLight(Vector3 direction, Color color);
    static void SetAmbientLight(float intensity);

private:
    static void InitShaders();
    static void InitSkybox();

    static RendererState s_State;
};
} // namespace CHEngine

#endif // CH_API_CONTEXT_H
