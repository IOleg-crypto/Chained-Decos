#include "engine/core/base.h"
#include "engine/graphics/renderer.h"
#include "engine/graphics/renderer2d.h"
#include "engine/graphics/scene_renderer.h"
#include "raylib.h"
#include "gtest/gtest.h"

using namespace CHEngine;

class RendererTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
#if defined(CH_CI)
        GTEST_SKIP() << "Skipping renderer tests on CI due to lack of reliable OpenGL support.";
#endif
    }

    void TearDown() override
    {
    }
};

#if !defined(CH_CI)

TEST_F(RendererTest, RendererInitialization)
{

    Renderer::Init();
    Renderer::Shutdown();
}

TEST_F(RendererTest, Renderer2DInitialization)
{
    Renderer2D renderer2d;
    renderer2d.Init();
    renderer2d.Shutdown();
}

TEST_F(RendererTest, Lifetime)
{

    Renderer::Init();
    Renderer::Shutdown();

    Renderer::Init();
    Renderer::Shutdown();
}

TEST_F(RendererTest, SceneRendererInitialization)
{
    SceneRenderer::Init();
    SceneRenderer::Shutdown();
}

#endif // CH_CI
