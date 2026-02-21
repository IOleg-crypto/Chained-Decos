#include "engine/audio/audio_importer.h"
#include "engine/core/base.h"
#include "engine/graphics/environment_importer.h"
#include "engine/graphics/font_importer.h"
#include "engine/graphics/mesh_importer.h"
#include "engine/graphics/shader_importer.h"
#include "raylib.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace CHEngine;

class ImporterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // HIDDEN window for raylib resource loading tests
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(1, 1, "ImporterTest");
        if (IsAudioDeviceReady() == false)
        {
            InitAudioDevice();
        }

        std::filesystem::create_directories("test_assets");
    }

    void TearDown() override
    {
        if (IsAudioDeviceReady())
        {
            CloseAudioDevice();
        }
        if (IsWindowReady())
        {
            CloseWindow();
        }

        std::filesystem::remove_all("test_assets");
    }
};

TEST_F(ImporterTest, EnvironmentImporter_SaveAndLoad)
{
    std::string testPath = "test_assets/test.chenv";

    auto env = std::make_shared<EnvironmentAsset>();
    env->GetSettings().Lighting.Ambient = 0.5f;
    env->GetSettings().Lighting.LightColor = RED;
    env->GetSettings().Fog.Enabled = true;

    // Test Saving
    EXPECT_TRUE(EnvironmentImporter::SaveEnvironment(env, testPath));
    EXPECT_TRUE(std::filesystem::exists(testPath));

    // Test Loading
    auto loadedEnv = EnvironmentImporter::ImportEnvironment(testPath);
    ASSERT_TRUE(loadedEnv);
    EXPECT_FLOAT_EQ(loadedEnv->GetSettings().Lighting.Ambient, 0.5f);

    Color expectedColor = RED;
    EXPECT_EQ(loadedEnv->GetSettings().Lighting.LightColor.r, expectedColor.r);
    EXPECT_TRUE(loadedEnv->GetSettings().Fog.Enabled);
}

TEST_F(ImporterTest, ShaderImporter_ParseConfig)
{
    std::string testPath = "test_assets/test.chshader";
    std::ofstream fs(testPath);
    fs << "Shader:\n";
    fs << "  VertexPath: \"vertex.glsl\"\n";
    fs << "  FragmentPath: \"fragment.glsl\"\n";
    fs.close();

    // Since we don't have real GLSL files here, we expect failure in actual loading,
    // but the importer should at least try to read the YAML.
    // For now, let's just assert it handles non-existent files gracefully.
    auto shader = ShaderImporter::ImportShader(testPath);
    EXPECT_FALSE(shader); // Should fail because glsl files don't exist
}

TEST_F(ImporterTest, FontImporter_InvalidPath)
{
    auto font = FontImporter::ImportFont("non_existent_font.ttf");
    EXPECT_FALSE(font);
}

TEST_F(ImporterTest, AudioImporter_InvalidPath)
{
    auto sound = AudioImporter::ImportSound("non_existent_sound.wav");
    EXPECT_FALSE(sound);
}

TEST_F(ImporterTest, MeshImporter_Procedural)
{
    auto cube = MeshImporter::GenerateProceduralModel(":cube:");
    EXPECT_GT(cube.meshCount, 0);
    EXPECT_NE(cube.meshes, nullptr);
    UnloadModel(cube);

    auto sphere = MeshImporter::GenerateProceduralModel(":sphere:");
    EXPECT_GT(sphere.meshCount, 0);
    UnloadModel(sphere);
}

TEST_F(ImporterTest, MeshImporter_InvalidProcedural)
{
    auto invalid = MeshImporter::GenerateProceduralModel(":invalid:");
    EXPECT_EQ(invalid.meshCount, 0);
}
