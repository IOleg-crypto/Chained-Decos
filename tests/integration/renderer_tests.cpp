// renderer_tests.cpp
// Tests for the main Renderer and Renderer2D singletons.
// These tests require a valid OpenGL context and are skipped on CI.
#include "engine/app/application.h"
#include <filesystem>
#include "engine/core/service_locator.h"
#include "engine/common/base.h"
#include "engine/graphics/pipeline/renderer.h"
#include "gtest/gtest.h"

using namespace Chained;

class RendererTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		if (!ServiceLocator::Has<Renderer>())
		{
			GTEST_SKIP() << "Renderer not available in headless mode";
		}
	}

	Renderer* GetRenderer()
	{
		return ServiceLocator::TryGet<Renderer>();
	}
};

// Verifies that the Renderer singleton can be initialized and shut down without errors.
TEST_F(RendererTest, RendererInitialization)
{
	EXPECT_TRUE(ServiceLocator::Has<Renderer>());
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
	camera.FovDegrees = 45.0f;
	camera.Projection = ProjectionType::Perspective;

	renderer->BeginScene(camera);

	EXPECT_FLOAT_EQ(renderer->GetFrame().CameraPosition.x, 10.0f);
	EXPECT_FLOAT_EQ(renderer->GetFrame().CameraPosition.y, 5.0f);
	EXPECT_FLOAT_EQ(renderer->GetFrame().CameraPosition.z, 2.0f);

	renderer->EndScene();
}

// Verifies that lights can be set, counted, and cleared on the renderer.
TEST_F(RendererTest, LightManagement)
{
	auto* renderer = GetRenderer();
	ASSERT_NE(renderer, nullptr);

	auto& lm = renderer->GetLightingManager();

	lm.Clear();
	EXPECT_EQ(lm.GetLighting().LightCount, 0);

	RenderLight light;
	light.color = {1.0f, 0.5f, 0.0f, 1.0f};
	light.intensity = 5.0f;
	light.position = {1.0f, 2.0f, 3.0f};

	lm.SetLight(0, light);
	lm.SetLightCount(1);

	EXPECT_EQ(lm.GetLighting().LightCount, 1);
	EXPECT_FLOAT_EQ(lm.GetLighting().Lights[0].intensity, 5.0f);
	EXPECT_FLOAT_EQ(lm.GetLighting().Lights[0].position.x, 1.0f);
	EXPECT_FLOAT_EQ(lm.GetLighting().Lights[0].color.g, 0.5f);

	lm.Clear();
	EXPECT_EQ(lm.GetLighting().LightCount, 0);
}

// Verifies that Frame.DiagnosticMode float can be pushed into the renderer's data block.
TEST_F(RendererTest, FrameDiagnosticMode)
{
	auto* renderer = GetRenderer();
	ASSERT_NE(renderer, nullptr);

	renderer->SetDiagnosticMode(2.0f);
	EXPECT_FLOAT_EQ(renderer->GetFrame().DiagnosticMode, 2.0f);

	renderer->SetDiagnosticMode(0.0f);
	EXPECT_FLOAT_EQ(renderer->GetFrame().DiagnosticMode, 0.0f);
}

// Verifies environment settings application
TEST_F(RendererTest, EnvironmentApplication)
{
	auto* renderer = GetRenderer();
	ASSERT_NE(renderer, nullptr);

	auto& lm = renderer->GetLightingManager();

	EnvironmentSettings env;
	env.Lighting.Ambient = 0.75f;
	env.Lighting.Exposure = 1.2f;

	lm.ApplyEnvironment(env);
	EXPECT_FLOAT_EQ(lm.GetLighting().CurrentLighting.Ambient, 0.75f);
	EXPECT_FLOAT_EQ(lm.GetLighting().CurrentLighting.Exposure, 1.2f);
}
