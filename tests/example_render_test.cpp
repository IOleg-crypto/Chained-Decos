// #include "gtest/gtest.h"
// #include "engine/graphics/pipeline/renderer.h"
// #include "engine/app/application.h"

// using namespace Chained;

// class ExampleRenderTest : public ::testing::Test
// {
// protected:
//     void SetUp() override
//     {
//         // Renderer is initialized by the global EngineEnvironment in test_environment.cpp
//     }
// };

// TEST_F(ExampleRenderTest, RendererStateVerification)
// {
//     auto* renderer = Application::Get().GetServiceRegistry().Get<Renderer>();
//     ASSERT_NE(renderer, nullptr);


//     EXPECT_EQ(renderer->GetData().LightCount, 0);


//     renderer->SetViewport(0, 0, 800, 600);
//     EXPECT_EQ(renderer->GetViewportWidth(), 800);
//     EXPECT_EQ(renderer->GetViewportHeight(), 600);
// }

// TEST_F(ExampleRenderTest, ShaderStorageInitialization)
// {
//     auto* renderer = Application::Get().GetServiceRegistry().Get<Renderer>();
//     ASSERT_NE(renderer, nullptr);

//     auto& shaderLib = renderer->GetShaderStorage();


//     EXPECT_TRUE(true);
// }
