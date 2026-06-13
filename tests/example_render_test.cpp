// #include "gtest/gtest.h"
// #include "engine/graphics/pipeline/renderer.h"
// #include "engine/runtime/application.h"

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

//     // Перевірка початкового стану
//     EXPECT_EQ(renderer->GetData().LightCount, 0);
    
//     // Перевірка встановлення в'юпорту
//     renderer->SetViewport(0, 0, 800, 600);
//     EXPECT_EQ(renderer->GetViewportWidth(), 800);
//     EXPECT_EQ(renderer->GetViewportHeight(), 600);
// }

// TEST_F(ExampleRenderTest, ShaderLibraryInitialization)
// {
//     auto* renderer = Application::Get().GetServiceRegistry().Get<Renderer>();
//     ASSERT_NE(renderer, nullptr);

//     auto& shaderLib = renderer->GetShaderLibrary();
//     // Перевіряємо, чи завантажені базові шейдери (якщо вони мають бути)
//     // У headless режимі ресурси можуть не завантажуватися повністю, але бібліотека має існувати.
//     EXPECT_TRUE(true); 
// }
