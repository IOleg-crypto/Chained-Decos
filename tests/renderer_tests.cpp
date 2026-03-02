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

TEST_F(RendererTest, LightManagement)
{

    Renderer::Init();

    EXPECT_EQ(Renderer::Get().GetData().LightCount, 0);

    RenderLight light;
    light.color[0] = 1.0f;
    light.intensity = 5.0f;

    Renderer::Get().SetLight(0, light);
    Renderer::Get().SetLightCount(1);

    EXPECT_EQ(Renderer::Get().GetData().LightCount, 1);
    EXPECT_FLOAT_EQ(Renderer::Get().GetData().Lights[0].intensity, 5.0f);

    Renderer::Get().ClearLights();
    EXPECT_EQ(Renderer::Get().GetData().LightCount, 0);

    Renderer::Shutdown();
}

TEST_F(RendererTest, DiagnosticMode)
{

    Renderer::Init();

    Renderer::Get().SetDiagnosticMode(2.0f);
    EXPECT_FLOAT_EQ(Renderer::Get().GetData().DiagnosticMode, 2.0f);

    Renderer::Shutdown();
}

TEST_F(RendererTest, EnvironmentApplication)
{

    Renderer::Init();

    EnvironmentSettings env;
    env.Lighting.Ambient = 0.5f;

    Renderer::Get().ApplyEnvironment(env);
    EXPECT_FLOAT_EQ(Renderer::Get().GetData().CurrentLighting.Ambient, 0.5f);

    Renderer::Shutdown();
}

#endif // CH_CI
