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
#if defined(CH_CI) && defined(CH_PLATFORM_WINDOWS)
        GTEST_SKIP() << "Skipping renderer tests on Windows CI due to lack of OpenGL support.";
#endif
        // HIDDEN window for renderer tests
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(1, 1, "RendererTest");
    }

    void TearDown() override
    {
        if (IsWindowReady())
        {
            CloseWindow();
        }
    }
};

TEST_F(RendererTest, RendererSingleton)
{
    // Test that Get() returns a valid instance after initialization
    Renderer::Init();

    // Multiple calls to Get() should return the same instance
    auto& instance1 = Renderer::Get();
    auto& instance2 = Renderer::Get();
    EXPECT_EQ(&instance1, &instance2);

    Renderer::Shutdown();
}

TEST_F(RendererTest, Renderer2DSingleton)
{
    Renderer2D::Init();

    auto& instance1 = Renderer2D::Get();
    auto& instance2 = Renderer2D::Get();
    EXPECT_EQ(&instance1, &instance2);

    Renderer2D::Shutdown();
}

TEST_F(RendererTest, SingletonLifetime)
{
    // Test Shutdown and Re-initialization
    Renderer::Init();
    auto& firstInstance = Renderer::Get();
    Renderer::Shutdown();

    Renderer::Init();
    auto& secondInstance = Renderer::Get();
    EXPECT_NE(&firstInstance, &secondInstance); // Should be a new instance
    Renderer::Shutdown();
}

TEST_F(RendererTest, LightManagement)
{
    Renderer::Init();
    auto& renderer = Renderer::Get();

    EXPECT_EQ(renderer.GetData().LightCount, 0);

    RenderLight light;
    light.color[0] = 1.0f;
    light.intensity = 5.0f;

    renderer.SetLight(0, light);
    renderer.SetLightCount(1);

    EXPECT_EQ(renderer.GetData().LightCount, 1);
    EXPECT_FLOAT_EQ(renderer.GetData().Lights[0].intensity, 5.0f);

    renderer.ClearLights();
    EXPECT_EQ(renderer.GetData().LightCount, 0);

    Renderer::Shutdown();
}

TEST_F(RendererTest, DiagnosticMode)
{
    Renderer::Init();
    auto& renderer = Renderer::Get();

    renderer.SetDiagnosticMode(2.0f);
    EXPECT_FLOAT_EQ(renderer.GetData().DiagnosticMode, 2.0f);

    Renderer::Shutdown();
}

TEST_F(RendererTest, EnvironmentApplication)
{
    Renderer::Init();
    auto& renderer = Renderer::Get();

    EnvironmentSettings env;
    env.Lighting.Ambient = 0.5f;

    renderer.ApplyEnvironment(env);
    EXPECT_FLOAT_EQ(renderer.GetData().CurrentLighting.Ambient, 0.5f);

    Renderer::Shutdown();
}
