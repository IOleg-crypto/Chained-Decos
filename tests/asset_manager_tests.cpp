#include "engine/asset_manager.h"
#include <gtest/gtest.h>

using namespace CH;

TEST(AssetManagerTest, Initialization)
{
    // AssetManager is a static class, so we test Init/Shutdown
    AssetManager::Init();
    // Since we don't have a mock filesystem easily available here,
    // we just verify it doesn't crash and state is consistent.
    AssetManager::Shutdown();
}

TEST(AssetManagerTest, PathHandling)
{
    // Test relative path logic if it was public/testable
    // For now, AssetManager handles project-relative paths.
}
