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

TEST_F(ServiceLocatorTest, ProvideUniquePtrAndGet)
{
    auto module = std::make_unique<TestModuleA>();
    auto* raw = module.get();
    ServiceLocator::Provide(std::move(module));

    EXPECT_EQ(ServiceLocator::Get<TestModuleA>(), raw);
}

TEST_F(ServiceLocatorTest, ProvideAfterLockIsRejectedWithoutLeak)
{
    ServiceLocator::Provide(new TestModuleA());
    ServiceLocator::Lock();

    bool shutdownCalled = false;
    auto rejected = std::make_unique<TestModuleB>();
    rejected->shutdownFlag = &shutdownCalled;
    ServiceLocator::Provide(std::move(rejected)); // unique_ptr destroys it on rejection

    EXPECT_FALSE(ServiceLocator::Has<TestModuleB>());
    // Rejected module was never registered, so Shutdown() must not be called on it.
    ServiceLocator::Shutdown();
    EXPECT_FALSE(shutdownCalled);
}

TEST_F(ServiceLocatorTest, DuplicateProvideKeepsOriginal)
{
    auto* original = new TestModuleA();
    ServiceLocator::Provide(original);
    ServiceLocator::Provide(std::make_unique<TestModuleA>()); // rejected + destroyed

    EXPECT_EQ(ServiceLocator::TryGet<TestModuleA>(), original);
}

TEST_F(ServiceLocatorTest, TryGetReturnsNullptrWhenDisabled)
{
    auto* module = new TestModuleA();
    ServiceLocator::Provide(module);
    module->SetEnabled(false);

    EXPECT_EQ(ServiceLocator::TryGet<TestModuleA>(), nullptr);
}

namespace
{
// Looks up TestModuleA (registered before it, shut down AFTER it in reverse order)
// and TestModuleB (registered after it, shut down BEFORE it) from within Shutdown().
class DependencyProbeModule : public EngineModule
{
public:
    bool* sawLiveDependency = nullptr;   // TestModuleA must still be reachable
    bool* sawDeadDependency = nullptr;   // TestModuleB must already be unavailable

    void Initialize() override
    {
    }
    void Shutdown() override
    {
        if (sawLiveDependency)
        {
            *sawLiveDependency = ServiceLocator::TryGet<TestModuleA>() != nullptr;
        }
        if (sawDeadDependency)
        {
            *sawDeadDependency = ServiceLocator::TryGet<TestModuleB>() != nullptr;
        }
    }
};
} // namespace

TEST_F(ServiceLocatorTest, ShutdownHidesAlreadyShutDownModules)
{
    bool liveDependencyReachable = false;
    bool deadDependencyReachable = true;

    ServiceLocator::Provide(new TestModuleA());
    auto* probe = new DependencyProbeModule();
    probe->sawLiveDependency = &liveDependencyReachable;
    probe->sawDeadDependency = &deadDependencyReachable;
    ServiceLocator::Provide(probe);
    ServiceLocator::Provide(new TestModuleB());

    ServiceLocator::Shutdown();

    // Reverse order: B shuts down first, then probe. From the probe's Shutdown(),
    // A (not yet shut down) is reachable, B (already shut down) is not.
    EXPECT_TRUE(liveDependencyReachable);
    EXPECT_FALSE(deadDependencyReachable);
}
