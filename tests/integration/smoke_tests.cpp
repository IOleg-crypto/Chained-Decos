#include "engine/app/application.h"
#include "engine/core/service_locator.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/assets/asset_manager.h"
#include "gtest/gtest.h"

using namespace Chained;

TEST(SmokeTest, ApplicationHeadlessInit)
{
    EXPECT_TRUE(ServiceLocator::IsAvailable());
    EXPECT_NE(ServiceLocator::TryGet<AssetManager>(), nullptr);
}

TEST(SmokeTest, SceneCreateAndDestroy)
{
    Scene scene;
    for (int i = 0; i < 10; ++i)
    {
        scene.CreateEntity("Smoke_" + std::to_string(i));
    }

    auto& reg = scene.GetRegistry();
    int count = 0;
    for (auto entity : reg.view<TagComponent>())
    {
        (void)entity;
        ++count;
    }
    EXPECT_EQ(count, 10);
}

TEST(SmokeTest, AssetManagerLoadDummy)
{
    auto* am = ServiceLocator::TryGet<AssetManager>();
    ASSERT_NE(am, nullptr);
    EXPECT_TRUE(am->GetAssetDirectory().empty() || std::filesystem::exists(am->GetAssetDirectory()));
}
