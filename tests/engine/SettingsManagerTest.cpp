#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "Game/Menu/SettingsManager.h"

class SettingsManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary config file for testing
        testConfigFile = "test_game.cfg";
        
        // Clean up any existing test file
        if (std::filesystem::exists(testConfigFile)) {
            std::filesystem::remove(testConfigFile);
        }
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

TEST_F(SettingsManagerTest, SkyboxGammaSettingsDefaultValues) {
    // Create empty config file to use defaults
    CreateTestConfigFile("");
    
    // Note: SettingsManager loads from "game.cfg" by default
    // For testing, we might need to modify SettingsManager to accept a config path
    // For now, test the getter methods with default values
    
    SettingsManager settings;
    
    // Test default values (gamma should be disabled by default)
    EXPECT_FALSE(settings.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(settings.GetSkyboxGammaValue(), 2.2f);
}

TEST_F(SettingsManagerTest, SkyboxGammaSettingsSetAndGet) {
    SettingsManager settings;
    
    // Test setting gamma enabled
    settings.SetSkyboxGammaEnabled(true);
    EXPECT_TRUE(settings.IsSkyboxGammaEnabled());
    
    settings.SetSkyboxGammaEnabled(false);
    EXPECT_FALSE(settings.IsSkyboxGammaEnabled());
    
    // Test setting gamma value
    settings.SetSkyboxGammaValue(1.8f);
    EXPECT_FLOAT_EQ(settings.GetSkyboxGammaValue(), 1.8f);
    
    settings.SetSkyboxGammaValue(2.5f);
    EXPECT_FLOAT_EQ(settings.GetSkyboxGammaValue(), 2.5f);
}

TEST_F(SettingsManagerTest, SkyboxGammaSettingsValueClamping) {
    SettingsManager settings;
    
    // Test that values are clamped to valid range (0.5 to 3.0)
    settings.SetSkyboxGammaValue(0.1f); // Below minimum
    EXPECT_GE(settings.GetSkyboxGammaValue(), 0.5f);
    
    settings.SetSkyboxGammaValue(5.0f); // Above maximum
    EXPECT_LE(settings.GetSkyboxGammaValue(), 3.0f);
    
    settings.SetSkyboxGammaValue(2.2f); // Valid value
    EXPECT_FLOAT_EQ(settings.GetSkyboxGammaValue(), 2.2f);
}

TEST_F(SettingsManagerTest, SkyboxGammaSettingsSaveAndLoad) {
    SettingsManager settings;
    
    // Set gamma settings
    settings.SetSkyboxGammaEnabled(true);
    settings.SetSkyboxGammaValue(2.4f);
    
    // Save settings
    settings.SaveSettings();
    
    // Create new SettingsManager and load
    SettingsManager loadedSettings;
    loadedSettings.LoadSettings();
    
    // Verify loaded values
    EXPECT_TRUE(loadedSettings.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(loadedSettings.GetSkyboxGammaValue(), 2.4f);
}

TEST_F(SettingsManagerTest, SkyboxGammaSettingsMultipleChanges) {
    SettingsManager settings;
    
    // Test multiple changes
    settings.SetSkyboxGammaEnabled(true);
    settings.SetSkyboxGammaValue(1.5f);
    EXPECT_TRUE(settings.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(settings.GetSkyboxGammaValue(), 1.5f);
    
    settings.SetSkyboxGammaEnabled(false);
    settings.SetSkyboxGammaValue(2.8f);
    EXPECT_FALSE(settings.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(settings.GetSkyboxGammaValue(), 2.8f);
}

