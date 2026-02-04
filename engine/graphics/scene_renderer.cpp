#include "scene_renderer.h"
#include "engine/scene/components.h"
#include "engine/core/profiler.h"
#include "engine/scene/project.h"
#include "render_command.h"
#include <raymath.h>

namespace CHEngine
{
    void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, Timestep ts, const DebugRenderFlags* debugFlags)
    {
        CH_PROFILE_FUNCTION();
        
        CH_CORE_ASSERT(scene, "Scene is null!");
        
        // 1. Environmental setup
        auto env = scene->GetSettings().Environment;
        if (env)
        {
            Render::ApplyEnvironment(env->GetSettings());
            Render::DrawSkybox(env->GetSettings().Skybox, camera);
        }

        // --- Update Profiler Stats ---
        ProfilerStats stats;
        stats.EntityCount = (uint32_t)scene->GetRegistry().storage<entt::entity>().size();
        Profiler::UpdateStats(stats);
        
        // 2. Scene rendering flow
        Render::BeginScene(camera);
        {
            RenderModels(scene, ts);
            
            if (debugFlags)
                RenderDebug(scene, debugFlags);
                
            RenderEditorIcons(scene, camera);
        }
        Render::EndScene();
    }

    void SceneRenderer::RenderModels(Scene* scene, Timestep ts)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<TransformComponent, ModelComponent>();
        
        float targetFPS = 30.0f;
        if (Project::GetActive())
            targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;
        float frameTime = 1.0f / (targetFPS > 0 ? targetFPS : 30.0f);

        for (auto entity : view)
        {
            auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);
            
            int currentFrame = 0;
            if (registry.all_of<AnimationComponent>(entity))
            {
                auto& anim = registry.get<AnimationComponent>(entity);
                if (anim.IsPlaying && model.Asset)
                {
                    int animCount = 0;
                    auto* anims = model.Asset->GetAnimations(&animCount);
                    if (anims && anim.CurrentAnimationIndex < animCount)
                    {
                        anim.FrameTimeCounter += ts.GetSeconds();
                        while (anim.FrameTimeCounter >= frameTime)
                        {
                            anim.CurrentFrame++;
                            anim.FrameTimeCounter -= frameTime;
                            if (anim.CurrentFrame >= anims[anim.CurrentAnimationIndex].frameCount)
                            {
                                if (anim.IsLooping) anim.CurrentFrame = 0;
                                else {
                                    anim.CurrentFrame = anims[anim.CurrentAnimationIndex].frameCount - 1;
                                    anim.IsPlaying = false;
                                    anim.FrameTimeCounter = 0;
                                    break;
                                }
                            }
                        }
                    }
                }
                currentFrame = anim.CurrentFrame;
                Render::DrawModel(model.ModelPath, transform.GetTransform(), {}, anim.CurrentAnimationIndex, currentFrame);
            }
            else
            {
                Render::DrawModel(model.ModelPath, transform.GetTransform());
            }
        }
    }

    void SceneRenderer::RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags)
    {
        // Debug visualization (colliders, etc)
        // This can be expanded as needed
    }

    void SceneRenderer::RenderEditorIcons(Scene* scene, const Camera3D& camera)
    {
        // Billboard-style icons for lights/cameras in editor
    }
}
