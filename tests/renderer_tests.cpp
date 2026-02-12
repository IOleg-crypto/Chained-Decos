#include "engine/graphics/renderer.h"
#include "engine/graphics/renderer2d.h"
#include "gtest/gtest.h"
#include "raylib.h"

using namespace CHEngine;

class RendererTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // HIDDEN window for renderer tests
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(1, 1, "RendererTest");
    }

    void TearDown() override
    {
        if (IsWindowReady()) CloseWindow();
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
