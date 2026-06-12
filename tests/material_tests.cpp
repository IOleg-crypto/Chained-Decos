// material_tests.cpp
// Tests for the MaterialInstance struct (engine/graphics/pipeline/material.h).
// These tests are pure-CPU and do not require an OpenGL context.
#include "engine/graphics/pipeline/renderer_types.h"
#include "gtest/gtest.h"

using namespace Chained;

// Verifies that a default-constructed Material has engine-standard default values:
// white albedo, metalness=0, roughness=0.5, alpha=1.
TEST(MaterialTest, DefaultInitialization)
{
    Material material;
    
    EXPECT_FLOAT_EQ(material.AlbedoColor.r, 1.0f);
    EXPECT_FLOAT_EQ(material.AlbedoColor.g, 1.0f);
    EXPECT_FLOAT_EQ(material.AlbedoColor.b, 1.0f);
    EXPECT_FLOAT_EQ(material.AlbedoColor.a, 1.0f);

    EXPECT_FLOAT_EQ(material.Metalness, 0.0f);
    EXPECT_FLOAT_EQ(material.Roughness, 0.5f);

    EXPECT_FALSE(material.Transparent);
    EXPECT_FLOAT_EQ(material.Alpha, 1.0f);
}

// Verifies that fields can be mutated and read back correctly.
TEST(MaterialTest, StateModification)
{
    Material material;
    
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
