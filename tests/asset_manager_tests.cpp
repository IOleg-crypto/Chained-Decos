#include "engine/core/base.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/scene/project.h"
#include "gtest/gtest.h"

#include <atomic>
#include <filesystem>
#include <string>

using namespace CHEngine;

namespace
{
class DummyAsset final : public Asset
{
public:
    DummyAsset()
        : Asset(GetStaticType())
    {
    }

    static AssetType GetStaticType()
    {
        return AssetType::None;
    }

    int OnLoadedCount = 0;
    int LoadCount = 0;
    std::string LastLoadedPath;

    void OnLoaded() override
    {
        ++OnLoadedCount;
    }
};

class CountingLoader final : public IAssetLoader
{
public:
    explicit CountingLoader(bool shouldSucceed)
        : m_ShouldSucceed(shouldSucceed)
    {
    }

    std::shared_ptr<Asset> Create() override
    {
        ++CreateCalls;
        return std::make_shared<DummyAsset>();
    }

    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override
    {
        ++LoadCalls;
        if (auto dummy = std::dynamic_pointer_cast<DummyAsset>(asset))
        {
            ++dummy->LoadCount;
            dummy->LastLoadedPath = resolvedPath;
        }

        return m_ShouldSucceed;
    }

    int CreateCalls = 0;
    int LoadCalls = 0;

private:
    bool m_ShouldSucceed = true;
};

std::string MakeUniqueAssetPath(const char* suffix)
{
    static std::atomic<uint64_t> counter{0};
    return std::string("unit_tests/") + suffix + "_" + std::to_string(++counter) + ".dummy";
}
} // namespace

class AssetManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto project = Project::New();
        project->GetConfig().ProjectDirectory = std::filesystem::current_path();
        project->GetConfig().AssetDirectory = "assets";
        Project::SetEngineRoot(project->GetConfig().ProjectDirectory);
    }

    static CountingLoader* RegisterDummyLoader(bool shouldSucceed)
    {
        auto loader = std::make_unique<CountingLoader>(shouldSucceed);
        CountingLoader* rawLoader = loader.get();
        AssetManager::Get().RegisterLoader(DummyAsset::GetStaticType(), std::move(loader));
        return rawLoader;
    }
};

TEST_F(AssetManagerTest, ResolvePathReturnsEmptyForEmptyInput)
{
    EXPECT_TRUE(AssetManager::Get().ResolvePath("").empty());
}

TEST_F(AssetManagerTest, GetCachesAssetAndLoadsOnlyOnce)
{
    CountingLoader* loader = RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("cache");

    auto first = AssetManager::Get().Get<DummyAsset>(path);
    auto second = AssetManager::Get().Get<DummyAsset>(path);

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first.get(), second.get());
    EXPECT_EQ(loader->CreateCalls, 1);
    EXPECT_EQ(loader->LoadCalls, 1);
    EXPECT_EQ(first->GetState(), AssetState::Ready);
    EXPECT_EQ(first->OnLoadedCount, 1);
    EXPECT_FALSE(first->LastLoadedPath.empty());
}

TEST_F(AssetManagerTest, ResolveToHandleReturnsValidHandleAfterLoad)
{
    RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("handle");

    auto loaded = AssetManager::Get().Get<DummyAsset>(path);
    ASSERT_NE(loaded, nullptr);

    AssetHandle handle = AssetManager::Get().ResolveToHandle(path);
    EXPECT_NE((uint64_t)handle, 0ull);

    auto byHandle = AssetManager::Get().Get<DummyAsset>(handle);
    ASSERT_NE(byHandle, nullptr);
    EXPECT_EQ(byHandle.get(), loaded.get());
}

TEST_F(AssetManagerTest, ReloadInvokesLoaderAgainForExistingAsset)
{
    CountingLoader* loader = RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("reload");

    auto asset = AssetManager::Get().Load<DummyAsset>(path);
    ASSERT_NE(asset, nullptr);
    ASSERT_EQ(loader->LoadCalls, 1);

    AssetManager::Get().Reload<DummyAsset>(path);

    EXPECT_EQ(loader->LoadCalls, 2);
    EXPECT_GE(asset->OnLoadedCount, 2);
}

TEST_F(AssetManagerTest, FailedLoadMarksAssetAsFailed)
{
    CountingLoader* loader = RegisterDummyLoader(false);
    const std::string path = MakeUniqueAssetPath("failed");

    auto asset = AssetManager::Get().Load<DummyAsset>(path);

    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(loader->LoadCalls, 1);
    EXPECT_EQ(asset->GetState(), AssetState::Failed);
    EXPECT_EQ(asset->OnLoadedCount, 0);
}
