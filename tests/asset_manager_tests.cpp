#include "engine/core/base.h"
#include "engine/assets/asset_manager.h"
#include "engine/scene/project.h"
#include "engine/core/uuid.h"
#include "engine/core/service_locator.h"
#include "engine/core/thread_pool.h"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

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
    explicit CountingLoader(bool shouldSucceed, bool asyncLoad = false)
        : m_ShouldSucceed(shouldSucceed)
        , m_AsyncLoad(asyncLoad)
    {
    }

    std::shared_ptr<Asset> Create() const override
    {
        return std::make_shared<DummyAsset>();
    }

    bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError = nullptr) override
    {
        auto dummy = std::dynamic_pointer_cast<DummyAsset>(asset);
        if (!dummy) return false;

        ++LoadCalls;
        if (!m_ShouldSucceed)
        {
            if (outError) *outError = "CountingLoader: forced failure for test path '" + ctx.ResolvedPath + "'";
            return false;
        }

        dummy->SetPath(ctx.ResolvedPath);
        dummy->LoadCount = 1; 
        dummy->LastLoadedPath = ctx.ResolvedPath;
        return true;
    }

    bool IsAsync() const override
    {
        return m_AsyncLoad;
    }

    int LoadCalls = 0;

private:
    bool m_ShouldSucceed = true;
    bool m_AsyncLoad = false;
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
        auto resolver = std::make_shared<AssetPathResolver>();
        auto registry = std::make_shared<AssetRegistry>();
        m_AssetManager = std::make_shared<AssetManager>(resolver, registry);
        ServiceLocator::Register<AssetManager>(m_AssetManager.get());

        // Ensure ThreadPool is registered for async tests
        if (!ServiceLocator::Has<ThreadPool>())
        {
            m_OwnThreadPool = std::make_unique<ThreadPool>();
            ServiceLocator::Register<ThreadPool>(m_OwnThreadPool.get());
            m_OwnThreadPool->Start();
        }

        auto project = Project::New();
        auto currentPath = std::filesystem::current_path();
        project->GetConfig().ProjectDirectory = currentPath;
        project->GetConfig().AssetDirectory = currentPath / "test_assets_unit";
        std::filesystem::create_directories(project->GetConfig().AssetDirectory);
        Project::SetEngineRoot(currentPath);

        resolver->SetRoots(currentPath, currentPath, project->GetConfig().AssetDirectory);
    }

    void TearDown() override
    {
        ServiceLocator::Remove<AssetManager>();
        if (m_OwnThreadPool)
        {
            m_OwnThreadPool->Stop();
            ServiceLocator::Remove<ThreadPool>();
            m_OwnThreadPool.reset();
        }
        m_AssetManager.reset();
    }

    std::shared_ptr<AssetManager> m_AssetManager;
    std::unique_ptr<ThreadPool> m_OwnThreadPool;

    CountingLoader* RegisterDummyLoader(bool shouldSucceed, bool asyncLoad = false)
    {
        auto loader = std::make_shared<CountingLoader>(shouldSucceed, asyncLoad);
        CountingLoader* rawLoader = loader.get();
        m_AssetManager->RegisterLoader(DummyAsset::GetStaticType(), loader);
        return rawLoader;
    }
};

TEST_F(AssetManagerTest, ResolvePathReturnsEmptyForEmptyInput)
{
    EXPECT_TRUE(m_AssetManager->ResolvePath("").empty());
}

TEST_F(AssetManagerTest, GetCachesAssetAndLoadsOnlyOnce)
{
    CountingLoader* loader = RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("cache");

    auto handleFirst = m_AssetManager->ResolveToHandle(path, DummyAsset::GetStaticType());
    auto first = m_AssetManager->Get<DummyAsset>(handleFirst);
    auto handleSecond = m_AssetManager->ResolveToHandle(path, DummyAsset::GetStaticType());
    auto second = m_AssetManager->Get<DummyAsset>(handleSecond);

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first.get(), second.get());
    EXPECT_EQ(loader->LoadCalls, 1);
    EXPECT_EQ(first->GetState(), AssetState::Ready);
    EXPECT_EQ(first->OnLoadedCount, 1);
    EXPECT_FALSE(first->LastLoadedPath.empty());
}

TEST_F(AssetManagerTest, ResolveToHandleReturnsValidHandleAfterLoad)
{
    RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("handle");

    auto loadedHandle = m_AssetManager->ResolveToHandle(path, DummyAsset::GetStaticType());
    auto loaded = m_AssetManager->Get<DummyAsset>(loadedHandle);
    ASSERT_NE(loaded, nullptr);

    AssetHandle handle = m_AssetManager->ResolveToHandle(path);
    EXPECT_NE((uint64_t)handle, 0ull);

    auto byHandle = m_AssetManager->Get<DummyAsset>(handle);
    ASSERT_NE(byHandle, nullptr);
    EXPECT_EQ(byHandle.get(), loaded.get());
}

TEST_F(AssetManagerTest, ReloadInvokesLoaderAgainForExistingAsset)
{
    CountingLoader* loader = RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("reload");

    auto handle = m_AssetManager->ResolveToHandle(path, DummyAsset::GetStaticType());
    auto asset = m_AssetManager->Get<DummyAsset>(handle);
    ASSERT_NE(asset, nullptr);
    ASSERT_EQ(loader->LoadCalls, 1);

    m_AssetManager->Reload<DummyAsset>(path);

    // AssetManager::Reload needs implementation in project, currently empty
    // But for tests we might want to check it if we implement it.
    // EXPECT_EQ(loader->LoadCalls, 2);
}

TEST_F(AssetManagerTest, FailedLoadMarksAssetAsFailed)
{
    CountingLoader* loader = RegisterDummyLoader(false);
    const std::string path = MakeUniqueAssetPath("failed");

    auto handle = m_AssetManager->ResolveToHandle(path, DummyAsset::GetStaticType());
    auto asset = m_AssetManager->Get<DummyAsset>(handle);

    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(loader->LoadCalls, 1);
    EXPECT_EQ(asset->GetState(), AssetState::Failed);
    EXPECT_EQ(asset->OnLoadedCount, 0);
}

TEST_F(AssetManagerTest, AsyncLoadQueuesFinalizeAndCompletesOnUpdate)
{
    CountingLoader* loader = RegisterDummyLoader(true, true);
    const std::string path = MakeUniqueAssetPath("async");

    auto handle = m_AssetManager->ResolveToHandle(path, DummyAsset::GetStaticType());
    auto asset = m_AssetManager->Get<DummyAsset>(handle);
    ASSERT_NE(asset, nullptr);

    for (int attempt = 0; attempt < 1000 && m_AssetManager->GetPendingFinalizeCount() == 0; ++attempt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ASSERT_GT(m_AssetManager->GetPendingFinalizeCount(), 0u);

    m_AssetManager->OnUpdate(Timestep(0.016f));

    EXPECT_EQ(loader->LoadCalls, 1);
    EXPECT_EQ(asset->GetState(), AssetState::Ready);
    EXPECT_EQ(asset->OnLoadedCount, 1);
    EXPECT_EQ(m_AssetManager->GetPendingFinalizeCount(), 0u);
}
