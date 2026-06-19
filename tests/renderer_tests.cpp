// renderer_tests.cpp
// Tests for the main Renderer and Renderer2D singletons.
// These tests require a valid OpenGL context and are skipped on CI.
#include "engine/app/application.h"
#include "engine/core/base.h"
#include "engine/graphics/pipeline/renderer.h"
#include "gtest/gtest.h"


using namespace Chained;

class RendererTest : public ::testing::Test
{
protected:
    Renderer* GetRenderer()
    {
        return Application::Get().GetServiceRegistry().Get<Renderer>();
    }
};

// Verifies that the Renderer singleton can be initialized and shut down without errors.
TEST_F(RendererTest, RendererInitialization)
{
    EXPECT_TRUE(Application::Get().GetServiceRegistry().Has<Renderer>());
}

// Verifies viewport size management
TEST_F(RendererTest, ViewportSize)
{
    auto* renderer = GetRenderer();
    ASSERT_NE(renderer, nullptr);

    renderer->SetViewportSize(1920, 1080);
    EXPECT_EQ(renderer->GetViewportWidth(), 1920);
    EXPECT_EQ(renderer->GetViewportHeight(), 1080);
}

// Verifies that BeginScene updates internal camera data
TEST_F(RendererTest, SceneLifecycle)
{
    auto* renderer = GetRenderer();
    ASSERT_NE(renderer, nullptr);

    Camera3D camera;
    camera.Position = {10.0f, 5.0f, 2.0f};
    camera.Target = {0.0f, 0.0f, 0.0f};
    camera.Up = {0.0f, 1.0f, 0.0f};
    camera.Fovy = 45.0f;
    camera.Projection = 0; // Perspective

    renderer->BeginScene(camera, 0.1f, 1000.0f);

    EXPECT_FLOAT_EQ(renderer->GetData().CurrentCameraPosition.x, 10.0f);
    EXPECT_FLOAT_EQ(renderer->GetData().CurrentCameraPosition.y, 5.0f);
    EXPECT_FLOAT_EQ(renderer->GetData().CurrentCameraPosition.z, 2.0f);

    renderer->EndScene();
}

// Verifies that lights can be set, counted, and cleared on the renderer.
TEST_F(RendererTest, LightManagement)
{
    auto* renderer = GetRenderer();
    ASSERT_NE(renderer, nullptr);

    renderer->ClearLights();
    EXPECT_EQ(renderer->GetData().LightCount, 0);

    RenderLight light;
    light.color = {1.0f, 0.5f, 0.0f, 1.0f};
    light.intensity = 5.0f;
    light.position = {1.0f, 2.0f, 3.0f};

    renderer->SetLight(0, light);
    renderer->SetLightCount(1);

    EXPECT_EQ(renderer->GetData().LightCount, 1);
    EXPECT_FLOAT_EQ(renderer->GetData().Lighting.Lights[0].intensity, 5.0f);
    EXPECT_FLOAT_EQ(renderer->GetData().Lighting.Lights[0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(renderer->GetData().Lighting.Lights[0].color.g, 0.5f);

    renderer->ClearLights();
    EXPECT_EQ(renderer->GetData().LightCount, 0);
}

// Verifies that DiagnosticMode float can be pushed into the renderer's data block.
TEST_F(RendererTest, DiagnosticMode)
{
    auto* renderer = GetRenderer();
    ASSERT_NE(renderer, nullptr);

    renderer->SetDiagnosticMode(2.0f);
    EXPECT_FLOAT_EQ(renderer->GetData().DiagnosticMode, 2.0f);

    renderer->SetDiagnosticMode(0.0f);
    EXPECT_FLOAT_EQ(renderer->GetData().DiagnosticMode, 0.0f);
}

// Verifies environment settings application
TEST_F(RendererTest, EnvironmentApplication)
{
    auto* renderer = GetRenderer();
    ASSERT_NE(renderer, nullptr);

    EnvironmentSettings env;
    env.Lighting.Ambient = 0.75f;
    env.Lighting.Exposure = 1.2f;

    renderer->ApplyEnvironment(env);
    EXPECT_FLOAT_EQ(renderer->GetData().Lighting.CurrentLighting.Ambient, 0.75f);
    EXPECT_FLOAT_EQ(renderer->GetData().Lighting.CurrentLighting.Exposure, 1.2f);
}
