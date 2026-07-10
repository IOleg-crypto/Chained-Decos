#include "engine/assets/loaders/assimp_importer.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace Chained;

class ImporterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        std::ofstream out("test_load.obj");
        if (!out.is_open()) {
            std::filesystem::create_directories("test_assets");
            out.open("test_load.obj");
        }
        out << "v -1.0 -1.0 0.0\n";
        out << "v 1.0 -1.0 0.0\n";
        out << "v 0.0 1.0 0.0\n";
        out << "f 1 2 3\n";
        out.close();
    }

    void TearDown() override
    {
        std::filesystem::remove("test_load.obj");
    }
};

TEST_F(ImporterTest, AssimpImporter_LoadBasicObj)
{
    auto data = AssimpImporter::Import("test_load.obj", 30);
    EXPECT_TRUE(data.isValid);
    EXPECT_EQ(data.meshes.size(), 1);
    if (!data.meshes.empty()) {
        EXPECT_EQ(data.meshes[0].vertices.size(), 3 * 3); // 3 vertices * 3 coords
        EXPECT_EQ(data.meshes[0].indices.size(), 3); // 1 face * 3 indices
    }
}

TEST_F(ImporterTest, AssimpImporter_InvalidPath)
{
    auto data = AssimpImporter::Import("non_existent_model.obj", 30);
    EXPECT_FALSE(data.isValid);
}
