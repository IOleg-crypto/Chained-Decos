#include "engine/core/engine_module.h"
#include "engine/core/service_locator.h"
#include "gtest/gtest.h"

using namespace Chained;

namespace
{
// The ServiceLocator OWNS provided modules and deletes them during Shutdown().
// Tests therefore route the shutdown flag through an external bool that outlives
// the module, so assertions after Shutdown() don't read freed memory.
class TestModuleA : public EngineModule
{
public:
    bool initialized = false;
    bool* shutdownFlag = nullptr;

    void Initialize() override
    {
        initialized = true;
    }
    void Shutdown() override
    {
        if (shutdownFlag)
        {
            *shutdownFlag = true;
        }
    }
};

class TestModuleB : public EngineModule
{
public:
    bool initialized = false;
    bool* shutdownFlag = nullptr;

    void Initialize() override
    {
        initialized = true;
    }
    void Shutdown() override
    {
        if (shutdownFlag)
        {
            *shutdownFlag = true;
        }
    }
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
    bool shutdownCalled = false;
    auto* module = new TestModuleA();
    module->shutdownFlag = &shutdownCalled;
    ServiceLocator::Provide(module);

    ServiceLocator::Shutdown();

    EXPECT_TRUE(shutdownCalled);
}

TEST_F(ServiceLocatorTest, ShutdownClearsServices)
{
    ServiceLocator::Provide(new TestModuleA());
    ServiceLocator::Shutdown();

    EXPECT_FALSE(ServiceLocator::IsAvailable());
}

TEST_F(ServiceLocatorTest, MultipleModulesShutdownInReverseOrder)
{
    bool shutdownA = false;
    bool shutdownB = false;

    auto* moduleA = new TestModuleA();
    moduleA->shutdownFlag = &shutdownA;
    ServiceLocator::Provide(moduleA);

    auto* moduleB = new TestModuleB();
    moduleB->shutdownFlag = &shutdownB;
    ServiceLocator::Provide(moduleB);

    ServiceLocator::Shutdown();

    EXPECT_TRUE(shutdownB);
    EXPECT_TRUE(shutdownA);
}
