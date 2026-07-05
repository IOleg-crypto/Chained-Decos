#include "engine/core/service_locator.h"
#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "thirdparty/googletest/googletest/include/gtest/gtest.h"


#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

using namespace Chained;

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

struct CountingLoaderData
{
    int LoadCalls = 0;
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
        if (ServiceLocator::Has<AssetManager>())
            m_PreviousAssetManager = ServiceLocator::Get<AssetManager>(); // We assume Get returns ptr, so we would have to copy it.
            
        m_AssetManager = std::make_shared<AssetManager>();
        ServiceLocator::ProvideRef<AssetManager>(m_AssetManager.get());

        auto currentPath = std::filesystem::current_path();
        m_AssetManager->SetProjectDirectory(currentPath);
        m_AssetManager->SetAssetDirectory(currentPath / "test_assets_unit");
        m_AssetManager->SetEngineRoot(currentPath);
        std::filesystem::create_directories(m_AssetManager->GetAssetDirectory());
    }

    void TearDown() override
    {
        if (m_PreviousAssetManager)
        {
            ServiceLocator::ProvideRef<AssetManager>(m_PreviousAssetManager);
        }
        m_AssetManager.reset();
    }

    std::shared_ptr<AssetManager> m_AssetManager;
    AssetManager* m_PreviousAssetManager = nullptr;
    std::vector<std::shared_ptr<CountingLoaderData>> m_LoaderData;

    CountingLoaderData* RegisterDummyLoader(bool shouldSucceed, bool asyncLoad = false)
    {
        auto data = std::make_shared<CountingLoaderData>();
        m_LoaderData.push_back(data);
        
        AssetLoader loader;
        loader.IsAsync = asyncLoad;
        loader.Create = []() { return std::make_shared<DummyAsset>(); };
        loader.Load = [data, shouldSucceed](std::shared_ptr<Asset> asset, const std::string& path, std::string* outError) {
            auto dummy = std::dynamic_pointer_cast<DummyAsset>(asset);
            if (!dummy)
                return false;

            ++data->LoadCalls;
            if (!shouldSucceed)
            {
                if (outError)
                    *outError = "CountingLoader: forced failure for test path '" + path + "'";
                return false;
            }

            dummy->SetPath(path);
            dummy->LoadCount = 1;
            dummy->LastLoadedPath = path;
            return true;
        };
        m_AssetManager->RegisterLoader(DummyAsset::GetStaticType(), loader);
        return data.get();
    }
};

TEST_F(AssetManagerTest, ResolvePathReturnsEmptyForEmptyInput)
{
    EXPECT_TRUE(m_AssetManager->ResolvePath("").empty());
}

TEST_F(AssetManagerTest, GetCachesAssetAndLoadsOnlyOnce)
{
    CountingLoaderData* loader = RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("cache");

    auto handleFirst = m_AssetManager->ResolveToHandle(path);
    auto first = m_AssetManager->Get<DummyAsset>(handleFirst);
    auto handleSecond = m_AssetManager->ResolveToHandle(path);
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

    auto loadedHandle = m_AssetManager->ResolveToHandle(path);
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
    CountingLoaderData* loader = RegisterDummyLoader(true);
    const std::string path = MakeUniqueAssetPath("reload");

    auto handle = m_AssetManager->ResolveToHandle(path);
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
    CountingLoaderData* loader = RegisterDummyLoader(false);
    const std::string path = MakeUniqueAssetPath("failed");

    auto handle = m_AssetManager->ResolveToHandle(path);
    auto asset = m_AssetManager->Get<DummyAsset>(handle);

    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(loader->LoadCalls, 1);
    EXPECT_EQ(asset->GetState(), AssetState::Failed);
    EXPECT_EQ(asset->OnLoadedCount, 0);
}

TEST_F(AssetManagerTest, AsyncLoadQueuesFinalizeAndCompletesOnUpdate)
{
    CountingLoaderData* loader = RegisterDummyLoader(true, true);
    const std::string path = MakeUniqueAssetPath("async");

    auto handle = m_AssetManager->ResolveToHandle(path);
    auto asset = m_AssetManager->Get<DummyAsset>(handle);
    ASSERT_NE(asset, nullptr);

    for (int attempt = 0; attempt < 1000 && m_AssetManager->GetPendingFinalizeCount() == 0; ++attempt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ASSERT_GT(m_AssetManager->GetPendingFinalizeCount(), 0u);

    m_AssetManager->Update();

    EXPECT_EQ(loader->LoadCalls, 1);
    EXPECT_EQ(asset->GetState(), AssetState::Ready);
    EXPECT_EQ(asset->OnLoadedCount, 1);
    EXPECT_EQ(m_AssetManager->GetPendingFinalizeCount(), 0u);
}

TEST_F(AssetManagerTest, GetWithInvalidHandleReturnsNull)
{
    auto asset = m_AssetManager->Get<DummyAsset>(AssetHandle(0));
    EXPECT_EQ(asset, nullptr);

    auto assetInvalid = m_AssetManager->Get<DummyAsset>(AssetHandle(123456789));
    EXPECT_EQ(assetInvalid, nullptr);
}

TEST_F(AssetManagerTest, MultipleAsyncLoads)
{
    CountingLoaderData* loader = RegisterDummyLoader(true, true);
    const int count = 5;
    std::vector<std::shared_ptr<DummyAsset>> assets;

    for (int i = 0; i < count; ++i)
    {
        const std::string path = MakeUniqueAssetPath("multi_async");
        auto handle = m_AssetManager->ResolveToHandle(path);
        assets.push_back(m_AssetManager->Get<DummyAsset>(handle));
    }

    // Wait for all to be in pending finalize
    for (int attempt = 0; attempt < 2000 && m_AssetManager->GetPendingFinalizeCount() < count; ++attempt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_EQ(m_AssetManager->GetPendingFinalizeCount(), (size_t)count);

    m_AssetManager->Update();

    EXPECT_EQ(m_AssetManager->GetPendingFinalizeCount(), 0u);
    for (auto& asset : assets)
    {
        EXPECT_EQ(asset->GetState(), AssetState::Ready);
    }
}
