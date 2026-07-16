#include "engine/core/layer_stack.h"
#include "engine/core/layer.h"
#include "gtest/gtest.h"

using namespace Chained;

TEST(LayerStackTest, EmptyOnInit)
{
    LayerStack stack;
    EXPECT_EQ(stack.begin(), stack.end());
    EXPECT_TRUE(stack.GetLayers().empty());
}

TEST(LayerStackTest, PushLayerAddsLayer)
{
    LayerStack stack;
    stack.PushLayer(std::make_unique<Layer>("TestLayer"));

    EXPECT_FALSE(stack.GetLayers().empty());
    EXPECT_EQ(stack.GetLayers().size(), 1u);
    EXPECT_EQ(stack.GetLayers()[0]->GetName(), "TestLayer");
}

TEST(LayerStackTest, PushOverlayAddsToEnd)
{
    LayerStack stack;
    stack.PushLayer(std::make_unique<Layer>("Layer1"));
    stack.PushLayer(std::make_unique<Layer>("Layer2"));
    stack.PushOverlay(std::make_unique<Layer>("Overlay"));

    ASSERT_EQ(stack.GetLayers().size(), 3u);
    EXPECT_EQ(stack.GetLayers()[0]->GetName(), "Layer1");
    EXPECT_EQ(stack.GetLayers()[1]->GetName(), "Layer2");
    EXPECT_EQ(stack.GetLayers()[2]->GetName(), "Overlay");
}

TEST(LayerStackTest, HasLayerFindsByName)
{
    LayerStack stack;
    stack.PushLayer(std::make_unique<Layer>("PlayerLayer"));

    EXPECT_TRUE(stack.HasLayer("PlayerLayer"));
    EXPECT_FALSE(stack.HasLayer("Nonexistent"));
}

TEST(LayerStackTest, LayerInsertIndex)
{
    LayerStack stack;
    stack.PushLayer(std::make_unique<Layer>("A"));
    stack.PushLayer(std::make_unique<Layer>("B"));
    stack.PushOverlay(std::make_unique<Layer>("O1"));
    stack.PushLayer(std::make_unique<Layer>("C"));

    ASSERT_EQ(stack.GetLayers().size(), 4u);
    EXPECT_EQ(stack.GetLayers()[0]->GetName(), "A");
    EXPECT_EQ(stack.GetLayers()[1]->GetName(), "B");
    EXPECT_EQ(stack.GetLayers()[2]->GetName(), "C");
    EXPECT_EQ(stack.GetLayers()[3]->GetName(), "O1");
}

TEST(LayerStackTest, ShutdownClearsLayers)
{
    LayerStack stack;
    stack.PushLayer(std::make_unique<Layer>("L1"));
    stack.PushLayer(std::make_unique<Layer>("L2"));

    stack.Shutdown();
    EXPECT_TRUE(stack.GetLayers().empty());
}
