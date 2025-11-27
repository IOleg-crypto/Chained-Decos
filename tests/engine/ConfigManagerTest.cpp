#include "core/config/Core/ConfigManager.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class ConfigManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a temporary config file for testing
        testConfigFile = "test_config.cfg";

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

TEST_F(ConfigManagerTest, SkyboxGammaSettingsDefaultValues)
{
    ConfigManager config;

    // Test default values
    EXPECT_FALSE(config.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(config.GetSkyboxGammaValue(), 2.2f);
}

TEST_F(ConfigManagerTest, SkyboxGammaSettingsSetAndGet)
{
    ConfigManager config;

    // Test setting gamma enabled
    config.SetSkyboxGammaEnabled(true);
    EXPECT_TRUE(config.IsSkyboxGammaEnabled());

    config.SetSkyboxGammaEnabled(false);
    EXPECT_FALSE(config.IsSkyboxGammaEnabled());

    // Test setting gamma value
    config.SetSkyboxGammaValue(1.8f);
    EXPECT_FLOAT_EQ(config.GetSkyboxGammaValue(), 1.8f);

    config.SetSkyboxGammaValue(2.5f);
    EXPECT_FLOAT_EQ(config.GetSkyboxGammaValue(), 2.5f);
}

TEST_F(ConfigManagerTest, SkyboxGammaSettingsLoadFromFile)
{
    // Create test config file with gamma settings
    CreateTestConfigFile("skybox_gamma_enabled = true\n"
                         "skybox_gamma_value = 2.0\n");

    ConfigManager config;
    EXPECT_TRUE(config.LoadFromFile(testConfigFile));

    // Verify loaded values
    EXPECT_TRUE(config.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(config.GetSkyboxGammaValue(), 2.0f);
}

TEST_F(ConfigManagerTest, SkyboxGammaSettingsSaveToFile)
{
    ConfigManager config;

    // Set gamma settings
    config.SetSkyboxGammaEnabled(true);
    config.SetSkyboxGammaValue(2.4f);

    // Save to file
    EXPECT_TRUE(config.SaveToFile(testConfigFile));

    // Load from file and verify
    ConfigManager loadedConfig;
    EXPECT_TRUE(loadedConfig.LoadFromFile(testConfigFile));

    EXPECT_TRUE(loadedConfig.IsSkyboxGammaEnabled());
    EXPECT_FLOAT_EQ(loadedConfig.GetSkyboxGammaValue(), 2.4f);
}

TEST_F(ConfigManagerTest, SkyboxGammaSettingsPartialConfig)
{
    // Test loading config with only gamma enabled
    CreateTestConfigFile("skybox_gamma_enabled = true\n");

    ConfigManager config;
    EXPECT_TRUE(config.LoadFromFile(testConfigFile));

    EXPECT_TRUE(config.IsSkyboxGammaEnabled());
    // Should use default value when not specified
    EXPECT_FLOAT_EQ(config.GetSkyboxGammaValue(), 2.2f);
}

TEST_F(ConfigManagerTest, SkyboxGammaSettingsInvalidValues)
{
    // Test with invalid values (should use defaults or handle gracefully)
    CreateTestConfigFile("skybox_gamma_enabled = invalid\n"
                         "skybox_gamma_value = not_a_number\n");

    ConfigManager config;
    EXPECT_TRUE(config.LoadFromFile(testConfigFile));

    // Should handle invalid values gracefully
    // Invalid bool should default to false
    EXPECT_FALSE(config.IsSkyboxGammaEnabled());
    // Invalid float should default to 2.2f
    EXPECT_FLOAT_EQ(config.GetSkyboxGammaValue(), 2.2f);
}
