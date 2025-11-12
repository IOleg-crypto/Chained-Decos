#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "Engine/Map/Skybox/Skybox.h"
#include "Engine/Config/Core/ConfigManager.h"

class SkyboxTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary config file for testing
        testConfigFile = "test_game.cfg";
        
        // Clean up any existing test file
        if (std::filesystem::exists(testConfigFile)) {
            std::filesystem::remove(testConfigFile);
        }
        
        // Create test config file
        CreateTestConfigFile(
            "skybox_gamma_enabled = false\n"
            "skybox_gamma_value = 2.2\n"
        );
    }

    void TearDown() override {
        // Clean up test file
        if (std::filesystem::exists(testConfigFile)) {
            std::filesystem::remove(testConfigFile);
        }
    }

    void CreateTestConfigFile(const std::string& content) {
        std::ofstream file(testConfigFile);
        file << content;
        file.close();
    }

    std::string testConfigFile;
};

TEST_F(SkyboxTest, ConstructorInitializesDefaults) {
    Skybox skybox;
    
    // Test default values
    EXPECT_FALSE(skybox.IsGammaEnabled());
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 2.2f);
    EXPECT_FALSE(skybox.IsInitialized());
    EXPECT_FALSE(skybox.IsLoaded());
}

TEST_F(SkyboxTest, InitInitializesSkybox) {
    Skybox skybox;
    
    // Init should initialize the skybox
    skybox.Init();
    
    EXPECT_TRUE(skybox.IsInitialized());
    // Note: IsLoaded() will be false until a texture is loaded
    EXPECT_FALSE(skybox.IsLoaded());
}

TEST_F(SkyboxTest, GammaSettingsSetAndGet) {
    Skybox skybox;
    skybox.Init();
    
    // Test setting gamma enabled
    skybox.SetGammaEnabled(true);
    EXPECT_TRUE(skybox.IsGammaEnabled());
    
    skybox.SetGammaEnabled(false);
    EXPECT_FALSE(skybox.IsGammaEnabled());
    
    // Test setting gamma value
    skybox.SetGammaValue(1.8f);
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 1.8f);
    
    skybox.SetGammaValue(2.5f);
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 2.5f);
}

TEST_F(SkyboxTest, GammaSettingsValueClamping) {
    Skybox skybox;
    skybox.Init();
    
    // Test that values are clamped to valid range (0.5 to 3.0)
    skybox.SetGammaValue(0.1f); // Below minimum
    EXPECT_GE(skybox.GetGammaValue(), 0.5f);
    
    skybox.SetGammaValue(5.0f); // Above maximum
    EXPECT_LE(skybox.GetGammaValue(), 3.0f);
    
    skybox.SetGammaValue(2.2f); // Valid value
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 2.2f);
}

TEST_F(SkyboxTest, UpdateGammaFromConfig) {
    Skybox skybox;
    skybox.Init();
    
    // Update gamma from config
    // Note: UpdateGammaFromConfig loads from "game.cfg" by default
    // This test verifies the method doesn't crash
    // In a real scenario, the config file would exist in the game directory
    EXPECT_NO_THROW(skybox.UpdateGammaFromConfig());
    
    // The gamma settings should remain at their current values
    // (either defaults or previously set values)
#if WIN32
    EXPECT_FALSE(std::isnan(skybox.GetGammaValue()));
#else
    EXPECT_FALSE(isnan(skybox.GetGammaValue()));
#endif
    EXPECT_GE(skybox.GetGammaValue(), 0.5f);
    EXPECT_LE(skybox.GetGammaValue(), 3.0f);
}

TEST_F(SkyboxTest, GammaSettingsBeforeInit) {
    Skybox skybox;
    
    // Test that gamma settings work before Init()
    skybox.SetGammaEnabled(true);
    skybox.SetGammaValue(2.0f);
    
    EXPECT_TRUE(skybox.IsGammaEnabled());
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 2.0f);
    
    // After Init(), settings should still be valid
    skybox.Init();
    EXPECT_TRUE(skybox.IsGammaEnabled());
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 2.0f);
}

TEST_F(SkyboxTest, GammaSettingsMultipleChanges) {
    Skybox skybox;
    skybox.Init();
    
    // Test multiple changes
    skybox.SetGammaEnabled(true);
    skybox.SetGammaValue(1.5f);
    EXPECT_TRUE(skybox.IsGammaEnabled());
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 1.5f);
    
    skybox.SetGammaEnabled(false);
    skybox.SetGammaValue(2.8f);
    EXPECT_FALSE(skybox.IsGammaEnabled());
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 2.8f);
    
    skybox.SetGammaEnabled(true);
    skybox.SetGammaValue(2.2f);
    EXPECT_TRUE(skybox.IsGammaEnabled());
    EXPECT_FLOAT_EQ(skybox.GetGammaValue(), 2.2f);
}

TEST_F(SkyboxTest, UpdateGammaFromConfigWithoutInit) {
    Skybox skybox;
    
    // UpdateGammaFromConfig should handle uninitialized skybox gracefully
    EXPECT_NO_THROW(skybox.UpdateGammaFromConfig());
    
    // After Init(), UpdateGammaFromConfig should work
    skybox.Init();
    EXPECT_NO_THROW(skybox.UpdateGammaFromConfig());
}

