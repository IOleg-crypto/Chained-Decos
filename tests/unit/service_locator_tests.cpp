#include "engine/core/service_locator.h"
#include "engine/core/engine_module.h"
#include "gtest/gtest.h"

using namespace Chained;

namespace
{
class TestModuleA : public EngineModule
{
public:
    bool initialized = false;
    bool shutdownCalled = false;

    void Initialize() override { initialized = true; }
    void Shutdown() override { shutdownCalled = true; }
};

class TestModuleB : public EngineModule
{
public:
    bool initialized = false;
    bool shutdownCalled = false;

    void Initialize() override { initialized = true; }
    void Shutdown() override { shutdownCalled = true; }
};
} // namespace

class ServiceLocatorTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        ServiceLocator::Shutdown();
    }
};

TEST_F(ServiceLocatorTest, ProvideAndGet)
{
    auto* module = new TestModuleA();
    ServiceLocator::Provide(module);

    auto* retrieved = ServiceLocator::Get<TestModuleA>();
    EXPECT_EQ(retrieved, module);
}

TEST_F(ServiceLocatorTest, HasReturnsTrueAfterProvide)
{
    ServiceLocator::Provide(new TestModuleA());
    EXPECT_TRUE(ServiceLocator::Has<TestModuleA>());
    EXPECT_FALSE(ServiceLocator::Has<TestModuleB>());
}

TEST_F(ServiceLocatorTest, TryGetReturnsNullptrWhenMissing)
{
    EXPECT_EQ(ServiceLocator::TryGet<TestModuleA>(), nullptr);
}

TEST_F(ServiceLocatorTest, TryGetReturnsPointerWhenProvided)
{
    auto* module = new TestModuleA();
    ServiceLocator::Provide(module);
    EXPECT_EQ(ServiceLocator::TryGet<TestModuleA>(), module);
}

TEST_F(ServiceLocatorTest, IsAvailableAfterProvide)
{
    EXPECT_FALSE(ServiceLocator::IsAvailable());
    ServiceLocator::Provide(new TestModuleA());
    EXPECT_TRUE(ServiceLocator::IsAvailable());
}

TEST_F(ServiceLocatorTest, ShutdownCallsShutdownOnModules)
{
    auto* module = new TestModuleA();
    ServiceLocator::Provide(module);

    auto* raw = ServiceLocator::Get<TestModuleA>();
    ServiceLocator::Shutdown();

    EXPECT_TRUE(raw->shutdownCalled);
}

TEST_F(ServiceLocatorTest, ShutdownClearsServices)
{
    ServiceLocator::Provide(new TestModuleA());
    ServiceLocator::Shutdown();

    EXPECT_FALSE(ServiceLocator::IsAvailable());
}

TEST_F(ServiceLocatorTest, MultipleModulesShutdownInReverseOrder)
{
    ServiceLocator::Provide(new TestModuleA());
    ServiceLocator::Provide(new TestModuleB());

    auto* rawA = ServiceLocator::Get<TestModuleA>();
    auto* rawB = ServiceLocator::Get<TestModuleB>();
    ServiceLocator::Shutdown();

    EXPECT_TRUE(rawB->shutdownCalled);
    EXPECT_TRUE(rawA->shutdownCalled);
}
