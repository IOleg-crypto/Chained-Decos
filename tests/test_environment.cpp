#include "gtest/gtest.h"
#include "engine/core/application.h"
#include "engine/graphics/renderer.h"
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

class EngineEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Run completely headless/hidden for tests
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        app = new TestApplication();
    }
    void TearDown() override {
        delete app;
    }
private:
    TestApplication* app = nullptr;
};

// Register the custom environment globally
::testing::Environment* const engine_env = ::testing::AddGlobalTestEnvironment(new EngineEnvironment);
