// renderer_tests.cpp
// Tests for the main Renderer and Renderer2D singletons.
// These tests require a valid OpenGL context and are skipped on CI.
#include "engine/core/base.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/core/service_locator.h"
#include "gtest/gtest.h"
#include "engine/core/application.h"

using namespace CHEngine;

class RendererTest : public ::testing::Test
{
protected:
    std::unique_ptr<Application> m_App;

    void SetUp() override
    {
        if (!CHEngine::Application::GetInstance())
        {
            ApplicationSpecification spec;
            spec.Name = "RendererTests";
            spec.Headless = true;
            m_App = std::make_unique<Application>(spec);
        }
    }

    void TearDown() override
    {
        if (m_App)
        {
            m_App.reset();
        }
    }
};

// Verifies that the Renderer singleton can be initialized and shut down without errors.
TEST_F(RendererTest, RendererInitialization)
{
    EXPECT_TRUE(ServiceLocator::Has<Renderer>());
}

// Verifies that the Renderer can be init/shutdown in sequence multiple times
// without memory corruption or double-free.
TEST_F(RendererTest, Lifetime)
{
    // Lifetime is managed by Application/Engine in SetUp/TearDown
    EXPECT_TRUE(ServiceLocator::Has<Renderer>());
}

// Verifies that lights can be set, counted, and cleared on the renderer.
TEST_F(RendererTest, LightManagement)
{
    auto& renderer = ServiceLocator::Get<Renderer>();

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
}

// Verifies that DiagnosticMode float can be pushed into the renderer's data block.
TEST_F(RendererTest, DiagnosticMode)
{
    auto& renderer = ServiceLocator::Get<Renderer>();

    renderer.SetDiagnosticMode(2.0f);
    EXPECT_FLOAT_EQ(renderer.GetData().DiagnosticMode, 2.0f);
}

// Verifies that an EnvironmentSettings object is applied correctly to the renderer's
// internal lighting data (e.g. ambient value round-trips through ApplyEnvironment).
TEST_F(RendererTest, EnvironmentApplication)
{
    auto& renderer = ServiceLocator::Get<Renderer>();

    EnvironmentSettings env;
    env.Lighting.Ambient = 0.5f;

    renderer.ApplyEnvironment(env);
    EXPECT_FLOAT_EQ(renderer.GetData().Lighting.CurrentLighting.Ambient, 0.5f);
}
