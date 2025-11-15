#include <gtest/gtest.h>
#include "Engine/Kernel/Core/Kernel.h"
#include "Engine/Kernel/Interfaces/IKernelService.h"
#include <memory>

// Mock service for testing
class MockService : public IKernelService {
public:
    MockService() : m_initialized(false) {}
    
    bool Initialize() override { 
        m_initialized = true; 
        return true;
    }
    void Shutdown() override { m_initialized = false; }
    void Update(float deltaTime) override { (void)deltaTime; }
    void Render() override {}
    const char* GetName() const override { return "MockService"; }
    
    bool IsInitialized() const { return m_initialized; }
    
private:
    bool m_initialized;
};

class KernelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a fresh kernel for each test
        kernel = std::make_unique<Kernel>();
        kernel->Initialize();
    }

    void TearDown() override {
        kernel->Shutdown();
        kernel.reset();
    }

    std::unique_ptr<Kernel> kernel;
};

TEST_F(KernelTest, GlobalInstanceAccess) {
    // After Initialize(), global instance should be available
    EXPECT_NO_THROW({
        Kernel& instance = Kernel::Instance();
        EXPECT_EQ(&instance, kernel.get());
    });
}

TEST_F(KernelTest, ServiceRegistrationAndRetrieval) {
    auto service = std::make_shared<MockService>();
    kernel->RegisterService<MockService>(service);
    
    auto retrieved = kernel->GetService<MockService>();
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(service, retrieved);
}

TEST_F(KernelTest, GetServiceReturnsNullForMissingService) {
    auto service = kernel->GetService<MockService>();
    EXPECT_EQ(service, nullptr);
}

TEST_F(KernelTest, HasServiceReturnsTrueForRegisteredService) {
    auto service = std::make_shared<MockService>();
    kernel->RegisterService<MockService>(service);
    
    EXPECT_TRUE(kernel->HasService<MockService>());
}

TEST_F(KernelTest, HasServiceReturnsFalseForMissingService) {
    EXPECT_FALSE(kernel->HasService<MockService>());
}

TEST_F(KernelTest, RequireServiceReturnsServiceWhenAvailable) {
    auto service = std::make_shared<MockService>();
    kernel->RegisterService<MockService>(service);
    
    EXPECT_NO_THROW({
        auto retrieved = kernel->RequireService<MockService>();
        EXPECT_NE(retrieved, nullptr);
        EXPECT_EQ(service, retrieved);
    });
}

TEST_F(KernelTest, RequireServiceThrowsWhenServiceMissing) {
    EXPECT_THROW({
        kernel->RequireService<MockService>();
    }, std::runtime_error);
}

TEST_F(KernelTest, GlobalMacrosWork) {
    auto service = std::make_shared<MockService>();
    kernel->RegisterService<MockService>(service);
    
    // Test KERNEL macro
    EXPECT_NO_THROW({
        Kernel& instance = KERNEL;
        EXPECT_EQ(&instance, kernel.get());
    });
    
    // Test GET_SERVICE macro
    auto retrieved = GET_SERVICE(MockService);
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(service, retrieved);
    
    // Test REQUIRE_SERVICE macro
    EXPECT_NO_THROW({
        auto required = REQUIRE_SERVICE(MockService);
        EXPECT_NE(required, nullptr);
        EXPECT_EQ(service, required);
    });
}

TEST_F(KernelTest, ShutdownClearsGlobalInstance) {
    // Verify instance is available
    EXPECT_NO_THROW({
        Kernel::Instance();
    });
    
    // Shutdown should clear the global instance
    kernel->Shutdown();
    kernel.reset();
    
    // Now Instance() should throw
    EXPECT_THROW({
        Kernel::Instance();
    }, std::runtime_error);
}
