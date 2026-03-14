#include "engine/core/base.h"
#include "engine/graphics/renderer.h"
#include "engine/graphics/renderer2d.h"
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
    Renderer renderer;
    renderer.Init();
    renderer.Shutdown();
}

TEST_F(RendererTest, Renderer2DInitialization)
{
    Renderer2D renderer2d;
    renderer2d.Init();
    renderer2d.Shutdown();
}

TEST_F(RendererTest, Lifetime)
{
    Renderer renderer;
    renderer.Init();
    renderer.Shutdown();

    renderer.Init();
    renderer.Shutdown();
}

TEST_F(RendererTest, LightManagement)
{
    Renderer renderer;
    renderer.Init();

    EXPECT_EQ(renderer.GetData().LightCount, 0);

    RenderLight light;
    light.color[0] = 1.0f;
    light.intensity = 5.0f;

    renderer.SetLight(0, light);
    renderer.SetLightCount(1);

    EXPECT_EQ(renderer.GetData().LightCount, 1);
    EXPECT_FLOAT_EQ(renderer.GetData().Lighting.Lights[0].intensity, 5.0f);

    renderer.ClearLights();
    EXPECT_EQ(renderer.GetData().LightCount, 0);

    renderer.Shutdown();
}

TEST_F(RendererTest, DiagnosticMode)
{
    Renderer renderer;
    renderer.Init();

    renderer.SetDiagnosticMode(2.0f);
    EXPECT_FLOAT_EQ(renderer.GetData().DiagnosticMode, 2.0f);

    renderer.Shutdown();
}

TEST_F(RendererTest, EnvironmentApplication)
{
    Renderer renderer;
    renderer.Init();

    EnvironmentSettings env;
    env.Lighting.Ambient = 0.5f;

    renderer.ApplyEnvironment(env);
    EXPECT_FLOAT_EQ(renderer.GetData().Lighting.CurrentLighting.Ambient, 0.5f);

    renderer.Shutdown();
}

#endif // CH_CI
