#include "core/config/config_manager.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class SettingsManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a temporary config file for testing
        testConfigFile = "test_game.cfg";

        // Clean up any existing test file
        if (std::filesystem::exists(testConfigFile))
        {
            std::filesystem::remove(testConfigFile);
        }
    }

    void TearDown() override
    {
        // Clean up test file
        if (std::filesystem::exists(testConfigFile))
        {
            std::filesystem::remove(testConfigFile);
        }
    }

    void CreateTestConfigFile(const std::string &content)
    {
        std::ofstream file(testConfigFile);
        file << content;
        file.close();
    }

    std::string testConfigFile;
};

TEST_F(SettingsManagerTest, SkyboxGammaSettingsDefaultValues)
{
    // Create empty config file to use defaults
    CreateTestConfigFile("");

    ConfigManager settings;
    // Load empty file to ensure defaults are set
    settings.LoadFromFile(testConfigFile);

    // Test default values
    EXPECT_FALSE(settings.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(settings.GetSkyboxGammaValue(), 2.2f); // Default for gamma is 2.2f
}

TEST_F(SettingsManagerTest, SkyboxGammaSettingsSetAndGet)
{
    ConfigManager settings;

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

TEST_F(SettingsManagerTest, SkyboxGammaSettingsSaveAndLoad)
{
    ConfigManager settings;

    // Set gamma settings
    settings.SetSkyboxGammaEnabled(true);
    settings.SetSkyboxGammaValue(2.4f);

    // Save settings
    settings.SaveToFile(testConfigFile);

    // Create new ConfigManager and load
    ConfigManager loadedSettings;
    loadedSettings.LoadFromFile(testConfigFile);

    // Verify loaded values
    EXPECT_TRUE(loadedSettings.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(loadedSettings.GetSkyboxGammaValue(), 2.4f);
}

TEST_F(SettingsManagerTest, SkyboxGammaSettingsMultipleChanges)
{
    ConfigManager settings;

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
