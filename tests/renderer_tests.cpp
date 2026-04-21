// renderer_tests.cpp
// Tests for the main Renderer and Renderer2D singletons.
// These tests require a valid OpenGL context and are skipped on CI.
#include "engine/core/base.h"
#include "engine/graphics/pipeline/renderer.h"
#include "gtest/gtest.h"
#include "engine/core/application.h"

using namespace CHEngine;

class RendererTest : public ::testing::Test
{
protected:
    std::unique_ptr<Application> m_App;

    void SetUp() override
    {
        ApplicationSpecification spec;
        spec.Name = "RendererTests";
        spec.Headless = true;
        m_App = std::make_unique<Application>(spec);
    }

    void TearDown() override
    {
        m_App.reset();
    }
};

// Verifies that the Renderer singleton can be initialized and shut down without errors.
TEST_F(RendererTest, RendererInitialization)
{
    Renderer::Init();
    Renderer::Shutdown();
}

// Verifies that the Renderer can be init/shutdown in sequence multiple times
// without memory corruption or double-free.
TEST_F(RendererTest, Lifetime)
{
    Renderer::Init();
    Renderer::Shutdown();

    Renderer::Init();
    Renderer::Shutdown();
}

// Verifies that lights can be set, counted, and cleared on the renderer.
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
    EXPECT_FLOAT_EQ(renderer.GetData().Lighting.Lights[0].intensity, 5.0f);

    renderer.ClearLights();
    EXPECT_EQ(renderer.GetData().LightCount, 0);

    Renderer::Shutdown();
}

// Verifies that DiagnosticMode float can be pushed into the renderer's data block.
TEST_F(RendererTest, DiagnosticMode)
{
    Renderer::Init();
    auto& renderer = Renderer::Get();

    renderer.SetDiagnosticMode(2.0f);
    EXPECT_FLOAT_EQ(renderer.GetData().DiagnosticMode, 2.0f);

    Renderer::Shutdown();
}

// Verifies that an EnvironmentSettings object is applied correctly to the renderer's
// internal lighting data (e.g. ambient value round-trips through ApplyEnvironment).
TEST_F(RendererTest, EnvironmentApplication)
{
    Renderer::Init();
    auto& renderer = Renderer::Get();

    EnvironmentSettings env;
    env.Lighting.Ambient = 0.5f;

    renderer.ApplyEnvironment(env);
    EXPECT_FLOAT_EQ(renderer.GetData().Lighting.CurrentLighting.Ambient, 0.5f);

    Renderer::Shutdown();
}
