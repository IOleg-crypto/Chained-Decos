// material_tests.cpp
// Tests for the MaterialInstance struct (engine/graphics/pipeline/material.h).
// These tests are pure-CPU and do not require an OpenGL context.
#include "engine/graphics/api/renderer_types.h"
#include "engine/graphics/pipeline/material.h"
#include "engine/assets/loaders/material_loader.h"
#include "gtest/gtest.h"

using namespace Chained;

// Verifies that a default-constructed Material has engine-standard default values:
// white albedo, metalness=0, roughness=0.5, alpha=1.
TEST(MaterialTest, DefaultInitialization)
{
    MaterialInstance material;

    EXPECT_EQ(material.AlbedoColor.r, 255);
    EXPECT_EQ(material.AlbedoColor.g, 255);
    EXPECT_EQ(material.AlbedoColor.b, 255);
    EXPECT_EQ(material.AlbedoColor.a, 255);

    EXPECT_FLOAT_EQ(material.Metalness, 0.0f);
    EXPECT_FLOAT_EQ(material.Roughness, 0.5f);

    EXPECT_FALSE(material.Transparent);
    EXPECT_FLOAT_EQ(material.Alpha, 1.0f);
}

// Verifies that fields can be mutated and read back correctly.
TEST(MaterialTest, StateModification)
{
    MaterialInstance material;

    AssetHandle testHandle(12345);
    material.AlbedoHandle = testHandle;

    material.Metalness = 0.8f;
    material.Roughness = 0.2f;
    material.Transparent = true;
    material.Alpha = 0.5f;

    EXPECT_EQ(material.AlbedoHandle, testHandle);
    EXPECT_FLOAT_EQ(material.Metalness, 0.8f);
    EXPECT_FLOAT_EQ(material.Roughness, 0.2f);
    EXPECT_TRUE(material.Transparent);
    EXPECT_FLOAT_EQ(material.Alpha, 0.5f);
}

TEST(MaterialTest, LoadMaterial)
{
    MaterialLoader loader;
    auto material = loader.Create();
    loader.Load(material, "resources/materials/default.mat");
    EXPECT_EQ(material->GetType(), AssetType::Material);
}
