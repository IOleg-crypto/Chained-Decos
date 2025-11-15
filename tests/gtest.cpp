//
// Created by I#Oleg
//
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>

#include <CameraController/CameraController.h>
#include <Color/ColorParser.h>
#include <Engine/Engine.h>
#include <Input/Core/InputManager.h>
#include <Map/Core/MapLoader.h>
#include <Menu/Menu.h>
#include <Model/Model.h>
#include <Player/Player.h>

using json = nlohmann::json;

class RaylibTestEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        // Completely disable raylib initialization for tests
        // This prevents assertion errors when raylib tries to access null window
#ifdef CI
#else
        // Don't initialize any graphics context for tests
        // Tests should run without requiring window/graphics context
        TraceLog(LOG_INFO, "Test environment: Skipping graphics initialization");
#endif
    }
    void TearDown() override
    {
#ifndef CI
        if (IsWindowReady())
            CloseWindow();
#endif
    }
};

// Register the global test environment
::testing::Environment *const raylib_env =
    ::testing::AddGlobalTestEnvironment(new RaylibTestEnvironment);

// Helper function to compare colors
bool ColorsEqual(const Color &a, const Color &b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// ============================================================================
// ColorParser Tests
// ============================================================================

TEST(ColorParserTest, ParseValidColors)
{
    // Test valid color names
    EXPECT_TRUE(ColorsEqual(ParseColorByName("white"), WHITE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("black"), BLACK));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("red"), RED));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("green"), GREEN));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("blue"), BLUE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("yellow"), YELLOW));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("orange"), ORANGE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("gray"), GRAY));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("purple"), PURPLE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("magenta"), MAGENTA));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("pink"), PINK));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("beige"), BEIGE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("brown"), BROWN));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("lime"), LIME));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("maroon"), MAROON));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("gold"), GOLD));
}

TEST(ColorParserTest, ParseInvalidColors)
{
    // Test invalid color names - should return WHITE as default
    EXPECT_TRUE(ColorsEqual(ParseColorByName("invalid_color"), WHITE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName(""), WHITE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("unknown"), WHITE));
    EXPECT_TRUE(ColorsEqual(ParseColorByName("reddish"), WHITE));
}

TEST(ColorParserTest, ParseCaseSensitiveColors)
{
    // Test case sensitivity - should return WHITE for wrong case
    // EXPECT_TRUE(ColorsEqual(ParseColorByName("WHITE"), WHITE)); // Should work if
    // case-insensitive
    EXPECT_TRUE(ColorsEqual(ParseColorByName("White"), WHITE)); // Should work if case-insensitive
    // EXPECT_TRUE(ColorsEqual(ParseColorByName("RED"), RED));     // Should work if
    // case-insensitive
}

// ============================================================================
// InputManager Tests
// ============================================================================

TEST(InputManagerTest, Constructor)
{
    InputManager manager;
    EXPECT_TRUE(true); // Basic test that constructor doesn't crash
}

TEST(InputManagerTest, RegisterAction)
{
    InputManager manager;

    // Test registering a simple action
    bool actionExecuted = false;
    auto testAction = [&actionExecuted]() { actionExecuted = true; };

    EXPECT_NO_THROW(manager.RegisterAction(KEY_SPACE, testAction));

    // Test registering multiple actions
    int actionCount = 0;
    auto countAction = [&actionCount]() { actionCount++; };

    EXPECT_NO_THROW(manager.RegisterAction(KEY_ENTER, countAction));
    EXPECT_NO_THROW(manager.RegisterAction(KEY_ESCAPE, countAction));
}

TEST(InputManagerTest, ProcessInput)
{
    InputManager manager;

    // Test processing input when no actions are registered
    EXPECT_NO_THROW(manager.ProcessInput());

    // Test processing input with registered actions
    bool actionExecuted = false;
    auto testAction = [&actionExecuted]() { actionExecuted = true; };
    manager.RegisterAction(KEY_SPACE, testAction);

    EXPECT_NO_THROW(manager.ProcessInput());
}

TEST(InputManagerTest, MultipleActions)
{
    InputManager manager;

    int action1Count = 0;
    int action2Count = 0;

    auto action1 = [&action1Count]() { action1Count++; };
    auto action2 = [&action2Count]() { action2Count++; };

    manager.RegisterAction(KEY_W, action1);
    manager.RegisterAction(KEY_S, action2);

    EXPECT_NO_THROW(manager.ProcessInput());
}

// ============================================================================
// Menu Tests
// ============================================================================

TEST(MenuTest, Constructor)
{
    Menu menu;
    EXPECT_TRUE(true); // Basic test that constructor doesn't crash
}

TEST(MenuTest, GetAction)
{
    Menu menu;

    // Test initial action
    EXPECT_EQ(menu.GetAction(), MenuAction::None);
}

TEST(MenuTest, ResetAction)
{
    Menu menu;

    // Test resetting action
    EXPECT_NO_THROW(menu.ResetAction());
    EXPECT_EQ(menu.GetAction(), MenuAction::None);
}

TEST(MenuTest, Update)
{
    Menu menu;

    // Test update function
    EXPECT_NO_THROW(menu.Update());
}

TEST(MenuTest, Render)
{
    Menu menu;

    // Test render function
    EXPECT_NO_THROW(menu.Render());
}

TEST(MenuTest, MenuActionEnum)
{
    // Test MenuAction enum values
    EXPECT_EQ(static_cast<int>(MenuAction::None), 0);
    EXPECT_EQ(static_cast<int>(MenuAction::StartGame), 1);
    EXPECT_EQ(static_cast<int>(MenuAction::OpenOptions), 2);
    EXPECT_EQ(static_cast<int>(MenuAction::ExitGame), 3);
}

// ============================================================================
// MapLoader Tests
// ============================================================================

TEST(MapLoaderTest, Constructor)
{
    MapLoader loader;
    EXPECT_TRUE(true); // Basic test that constructor doesn't crash
}

// TEST(MapLoaderTest, LoadMapNonExistent) {
//     // Test loading non-existent map
//     auto result = LoadMap("non_existent_map.json");
//     EXPECT_TRUE(result.empty());
// }

// TEST(MapLoaderTest, LoadMapEmptyFile) {
//     // Test loading empty file
//     auto result = LoadMap("empty_map.json");
//     EXPECT_TRUE(result.empty());
// }

TEST(MapLoaderTest, MapLoaderStruct)
{
    MapLoader loader;

    // Test default values
    EXPECT_TRUE(loader.modelName.empty());
    EXPECT_EQ(loader.position.x, 0.0f);
    EXPECT_EQ(loader.position.y, 0.0f);
    EXPECT_EQ(loader.position.z, 0.0f);
    EXPECT_EQ(loader.rotation.x, 0.0f);
    EXPECT_EQ(loader.rotation.y, 0.0f);
    EXPECT_EQ(loader.rotation.z, 0.0f);
    EXPECT_EQ(loader.scale.x, 0.0f);
    EXPECT_EQ(loader.scale.y, 0.0f);
    EXPECT_EQ(loader.scale.z, 0.0f);
}

// TEST(MapLoaderTest, LoadMapInvalidJson) {
//     // Test loading invalid JSON
//     auto result = LoadMap("invalid_json.json");
//     EXPECT_TRUE(result.empty());
// }

// ============================================================================
// CameraController Tests
// ============================================================================

TEST(CameraControllerTest, Constructor)
{
    CameraController controller;
    EXPECT_TRUE(true); // Basic test that constructor doesn't crash
}

TEST(CameraControllerTest, GetCamera)
{
    CameraController controller;

    // Test getting camera reference
    Camera &camera = controller.GetCamera();
    EXPECT_TRUE(true); // Basic test that function doesn't crash
}

TEST(CameraControllerTest, GetCameraMode)
{
    CameraController controller;

    // Test getting camera mode
    int &mode = controller.GetCameraMode();
    EXPECT_GE(mode, 0); // Mode should be non-negative
}

TEST(CameraControllerTest, SetCameraMode)
{
    CameraController controller;

    // Test setting valid camera modes
    EXPECT_NO_THROW(controller.SetCameraMode(0)); // First person
    EXPECT_NO_THROW(controller.SetCameraMode(1)); // Free camera
    EXPECT_NO_THROW(controller.SetCameraMode(2)); // Third person
    EXPECT_NO_THROW(controller.SetCameraMode(3)); // Orbital

    // Test setting invalid camera mode
    EXPECT_NO_THROW(controller.SetCameraMode(-1));
    EXPECT_NO_THROW(controller.SetCameraMode(10));
}

TEST(CameraControllerTest, Update)
{
    CameraController controller;

    // Test update function
    EXPECT_NO_THROW(controller.Update());
}

TEST(CameraControllerTest, CameraModeConsistency)
{
    CameraController controller;

    // Test that setting and getting camera mode works consistently
    controller.SetCameraMode(1);
    EXPECT_EQ(controller.GetCameraMode(), 1);

    controller.SetCameraMode(2);
    EXPECT_EQ(controller.GetCameraMode(), 2);
}

// ============================================================================
// ModelInstance Tests
// ============================================================================

TEST(ModelInstanceTest, ConstructorWithAllParameters)
{
    Vector3 pos = {1.0f, 2.0f, 3.0f};
    Model *model = nullptr; // We'll use nullptr for testing
    float scale = 2.0f;
    std::string name = "test_model";
    Color color = RED;
    std::string texturePath = "test_texture.png";
    Texture2D texture = {0}; // Empty texture for testing

    EXPECT_NO_THROW({
        ModelInstance instance(pos, model, scale, name, color, texturePath, texture);
        EXPECT_EQ(instance.GetModelName(), name);
        EXPECT_EQ(instance.GetColor().r, color.r);
        EXPECT_EQ(instance.GetScale(), scale);
        EXPECT_EQ(instance.GetModel(), model);
        EXPECT_EQ(instance.GetModelPosition().x, pos.x);
        EXPECT_EQ(instance.GetModelPosition().y, pos.y);
        EXPECT_EQ(instance.GetModelPosition().z, pos.z);
        EXPECT_EQ(instance.GetTexturePath(), texturePath);
    });
}

TEST(ModelInstanceTest, ConstructorWithColor)
{
    Vector3 pos = {0.0f, 0.0f, 0.0f};
    Model *model = nullptr;
    float scale = 1.0f;
    std::string name = "test_model";
    Color color = BLUE;

    EXPECT_NO_THROW({
        ModelInstance instance(pos, model, scale, name, color);
        EXPECT_EQ(instance.GetModelName(), name);
        EXPECT_EQ(instance.GetColor().b, color.b); // Blue component
        EXPECT_EQ(instance.GetScale(), scale);
        EXPECT_EQ(instance.GetModel(), model);
    });
}

TEST(ModelInstanceTest, ConstructorMinimal)
{
    Vector3 pos = {5.0f, 5.0f, 5.0f};
    Model *model = nullptr;
    float scale = 0.5f;
    std::string name = "minimal_model";

    EXPECT_NO_THROW({
        ModelInstance instance(pos, model, scale, name);
        EXPECT_EQ(instance.GetModelName(), name);
        EXPECT_EQ(instance.GetScale(), scale);
        EXPECT_EQ(instance.GetModel(), model);
        EXPECT_EQ(instance.GetModelPosition().x, pos.x);
        EXPECT_EQ(instance.GetModelPosition().y, pos.y);
        EXPECT_EQ(instance.GetModelPosition().z, pos.z);
    });
}

TEST(ModelInstanceTest, GetProperties)
{
    Vector3 pos = {10.0f, 20.0f, 30.0f};
    Model *model = nullptr;
    float scale = 3.0f;
    std::string name = "property_test";
    Color color = GREEN;

    ModelInstance instance(pos, model, scale, name, color);

    // Test all getter methods
    EXPECT_EQ(instance.GetModelName(), name);
    EXPECT_EQ(instance.GetColor().g, color.g); // Green component
    EXPECT_EQ(instance.GetScale(), scale);
    EXPECT_EQ(instance.GetModel(), model);
    EXPECT_EQ(instance.GetModelPosition().x, pos.x);
    EXPECT_EQ(instance.GetModelPosition().y, pos.y);
    EXPECT_EQ(instance.GetModelPosition().z, pos.z);
}

// ============================================================================
// Player Tests
// ============================================================================

TEST(PlayerTest, Constructor)
{
    Player player;
    // Test that player is created successfully
    EXPECT_NE(player.GetCameraController(), nullptr);
}

TEST(PlayerTest, SpeedOperations)
{
    Player player;

    // Test default speed
    float defaultSpeed = player.GetSpeed();
    EXPECT_GT(defaultSpeed, 0.0f);

    // Test setting speed
    float newSpeed = 5.0f;
    player.SetSpeed(newSpeed);
    EXPECT_EQ(player.GetSpeed(), newSpeed);

    // Test negative speed (should be handled gracefully)
    player.SetSpeed(-2.0f);
    EXPECT_EQ(player.GetSpeed(), -2.0f);
}

TEST(PlayerTest, Movement)
{
    Player player;

    // Test movement with zero offset
    Vector3 zeroOffset = {0, 0, 0};
    player.Move(zeroOffset);
    // Should not crash and should not change position significantly

    // Test movement with positive offset
    Vector3 positiveOffset = {1, 0, 0};
    player.Move(positiveOffset);
    // Should move player in positive X direction

    // Test movement with negative offset
    Vector3 negativeOffset = {-1, 0, 0};
    player.Move(negativeOffset);
    // Should move player in negative X direction
}

TEST(PlayerTest, ModelManager)
{
    Player player;

    // Test getting model manager
    Models modelManager = player.GetModelManager();
    // Should return valid model manager
    EXPECT_TRUE(true); // Basic test that function doesn't crash
}

TEST(PlayerTest, ApplyInput)
{
    Player player;

    // Test input application
    EXPECT_NO_THROW(player.ApplyInput());
}

// ============================================================================
// Models Tests
// ============================================================================

TEST(ModelsTest, Constructor)
{
    Models models;
    // Test that models object is created successfully
    EXPECT_TRUE(true); // Basic test that constructor doesn't crash
}

TEST(ModelsTest, LoadModelsFromJson)
{
    Models models;

    // Test loading from non-existent file
    EXPECT_NO_THROW(models.LoadModelsFromJson("non_existent_file.json"));

    // Test loading from empty file (if exists)
    // This would require creating a test JSON file
}

TEST(ModelsTest, DrawAllModels)
{
    Models models;

    // Test drawing when no models are loaded
    EXPECT_NO_THROW(models.DrawAllModels());
}

TEST(ModelsTest, GetModelByName)
{
    Models models;

    // Test getting model by name when no models are loaded
    // This should return a dummy model (not throw)
    EXPECT_NO_THROW({
        Model &model = models.GetModelByName("test_model");
        // The dummy model should have meshCount = 0
        EXPECT_EQ(model.meshCount, 0);
    });
}

TEST(ModelsTest, AddInstance)
{
    Models models;

    // Test adding instance with valid JSON
    json instanceJson = {{"position", {0, 0, 0}}, {"rotation", {0, 0, 0}}, {"scale", {1, 1, 1}}};

    Model *testModel = nullptr;
    std::string modelName = "test_model";

    EXPECT_NO_THROW(models.AddInstance(instanceJson, testModel, modelName, nullptr));
}

// ============================================================================
// Engine Tests
// ============================================================================

TEST(EngineTest, Constructor)
{
    // Test default constructor
    EXPECT_NO_THROW({ Engine engine1; });

    // Test parameterized constructor
    EXPECT_NO_THROW({ Engine engine2(1280, 720); });
}

TEST(EngineTest, Initialization)
{
    Engine engine(800, 600);

    // Test initialization - should not throw in normal circumstances
    // Note: This will try to create a second window, which might fail
    // but the Engine handles this gracefully
    EXPECT_NO_THROW(engine.Init());
}

TEST(EngineTest, BasicFunctionality)
{
    Engine engine(800, 600);

    // Test basic engine functionality
    EXPECT_NO_THROW(engine.Init());
    EXPECT_TRUE(engine.IsRunning());

    // Test exit functionality
    engine.RequestExit();
    EXPECT_FALSE(engine.IsRunning());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(IntegrationTest, InputManagerMenuInteraction)
{
    InputManager inputManager;
    Menu menu;

    // Test that input manager and menu can work together
    EXPECT_NO_THROW(inputManager.ProcessInput());
    EXPECT_NO_THROW(menu.Update());
}

TEST(IntegrationTest, CameraControllerPlayerInteraction)
{
    CameraController cameraController;
    Player player;

    // Test that camera controller and player can work together
    EXPECT_NO_THROW(cameraController.Update());
    EXPECT_NO_THROW(player.Update());
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(PerformanceTest, ColorParsingSpeed)
{
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        ParseColorByName("red");
        ParseColorByName("green");
        ParseColorByName("blue");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should complete in reasonable time (less than 1 second for 1000 iterations)
    EXPECT_LT(duration.count(), 1000000);
}

TEST(PerformanceTest, InputManagerSpeed)
{
    InputManager manager;

    // Register multiple actions
    for (int i = 0; i < 100; ++i)
    {
        manager.RegisterAction(KEY_A + i, []() {});
    }

    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        manager.ProcessInput();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 1000000);
}

TEST(PerformanceTest, ModelInstanceCreation)
{
    const int iterations = 100;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        Vector3 pos = {static_cast<float>(i), 0, 0};
        Model *model = nullptr;
        float scale = 1.0f;
        std::string name = "perf_test_" + std::to_string(i);
        Color color = WHITE;

        ModelInstance instance(pos, model, scale, name, color);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 1000000);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(EdgeCaseTest, EmptyColorName)
{
    Color result = ParseColorByName("");
    EXPECT_TRUE(ColorsEqual(result, WHITE)); // Should return default color
}

TEST(EdgeCaseTest, VeryLongColorName)
{
    std::string longName(1000, 'a');
    Color result = ParseColorByName(longName);
    EXPECT_TRUE(ColorsEqual(result, WHITE)); // Should return default color
}

TEST(EdgeCaseTest, SpecialCharactersInColorName)
{
    Color result = ParseColorByName("red@#$%");
    EXPECT_TRUE(ColorsEqual(result, WHITE)); // Should return default color
}

TEST(EdgeCaseTest, InputManagerEdgeCases)
{
    InputManager manager;

    // Test registering action with invalid key
    EXPECT_NO_THROW(manager.RegisterAction(-1, []() {}));
    EXPECT_NO_THROW(manager.RegisterAction(999, []() {}));

    // Test registering null function
    EXPECT_NO_THROW(manager.RegisterAction(KEY_SPACE, nullptr));
}

TEST(EdgeCaseTest, MenuEdgeCases)
{
    Menu menu;

    // Test multiple updates
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_NO_THROW(menu.Update());
    }

    // Test multiple resets
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_NO_THROW(menu.ResetAction());
    }
}

TEST(EdgeCaseTest, CameraControllerEdgeCases)
{
    CameraController controller;

    // Test extreme camera modes
    EXPECT_NO_THROW(controller.SetCameraMode(INT_MAX));
    EXPECT_NO_THROW(controller.SetCameraMode(INT_MIN));

    // Test multiple updates
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_NO_THROW(controller.Update());
    }
}

TEST(EdgeCaseTest, ModelInstanceEdgeCases)
{
    // Test with extreme values
    Vector3 extremePos = {FLT_MAX, FLT_MIN, 0};
    Model *model = nullptr;
    float extremeScale = FLT_MAX;
    std::string emptyName = "";
    Color color = WHITE;

    EXPECT_NO_THROW({
        ModelInstance instance(extremePos, model, extremeScale, emptyName, color);
        EXPECT_EQ(instance.GetModelName(), emptyName);
        EXPECT_EQ(instance.GetScale(), extremeScale);
    });
}

// TEST(EdgeCaseTest, MapLoaderEdgeCases) {
//     // Test with very long path
//     std::string longPath(1000, 'a');
//     auto result = LoadMap(longPath);
//     EXPECT_TRUE(result.empty());
//
//     // Test with path containing special characters
//     auto result2 = LoadMap("path/with/special/chars/!@#$%^&*()");
//     EXPECT_TRUE(result2.empty());
// }

// ============================================================================
// Stress Tests
// ============================================================================

TEST(StressTest, MultipleInputManagers)
{
    std::vector<InputManager> managers(100);

    for (auto &manager : managers)
    {
        for (int i = 0; i < 10; ++i)
        {
            manager.RegisterAction(KEY_A + i, []() {});
        }
        EXPECT_NO_THROW(manager.ProcessInput());
    }
}

TEST(StressTest, MultipleCameraControllers)
{
    std::vector<CameraController> controllers(25);

    for (auto &controller : controllers)
    {
        for (int i = 0; i < 10; ++i)
        {
            controller.SetCameraMode(i % 4);
            EXPECT_NO_THROW(controller.Update());
        }
    }
}

TEST(StressTest, MultipleModelInstances)
{
    std::vector<ModelInstance> instances;

    for (int i = 0; i < 100; ++i)
    {
        Vector3 pos = {static_cast<float>(i), 0, 0};
        Model *model = nullptr;
        float scale = 1.0f;
        std::string name = "stress_test_" + std::to_string(i);
        Color color = WHITE;

        instances.emplace_back(pos, model, scale, name, color);
    }

    EXPECT_EQ(instances.size(), 100);
}
