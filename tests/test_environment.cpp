// test_environment.cpp
// Global gtest Environment that boots a minimal headless Application before any test runs,
// then tears it down after all tests complete. This provides a valid engine context
// (log system, event bus, etc.) without showing a window.
#include "gtest/gtest.h"
#include "engine/core/application.h"
#include "engine/graphics/pipeline/renderer.h"
#include "raylib.h"

class TestApplication : public CHEngine::Application
{
public:
    TestApplication() : CHEngine::Application([](){
        CHEngine::ApplicationSpecification spec{"Engine Tests"};
        spec.Headless = true;
        return spec;
    }()) {}
};

// GlobalTestEnvironment: SetUp() is called once before the first test,
// TearDown() is called once after the last test.
class EngineEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Run completely headless/hidden for tests — no window on screen
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        app = new TestApplication();
    }
    void TearDown() override {
        delete app;
    }
private:
    TestApplication* app = nullptr;
};

// Register the custom environment globally so all test suites share the same engine boot.
::testing::Environment* const engine_env = ::testing::AddGlobalTestEnvironment(new EngineEnvironment);
