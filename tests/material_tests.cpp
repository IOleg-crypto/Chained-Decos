#include "engine/graphics/material.h"
#include "gtest/gtest.h"

using namespace CHEngine;

TEST(MaterialTest, DefaultInitialization)
{
    MaterialInstance material;
    
    // Check default values
    EXPECT_EQ(material.AlbedoColor.r, 255);
    EXPECT_EQ(material.AlbedoColor.g, 255);
    EXPECT_EQ(material.AlbedoColor.b, 255);
    EXPECT_EQ(material.AlbedoColor.a, 255);

    EXPECT_FALSE(material.OverrideAlbedo);
    EXPECT_FALSE(material.OverrideNormal);
    EXPECT_FALSE(material.OverrideMetallicRoughness);
    EXPECT_FALSE(material.OverrideOcclusion);
    EXPECT_FALSE(material.OverrideEmissive);
    EXPECT_FALSE(material.OverrideShader);

    EXPECT_FLOAT_EQ(material.Metalness, 0.0f);
    EXPECT_FLOAT_EQ(material.Roughness, 0.5f);

    EXPECT_FALSE(material.DoubleSided);
    EXPECT_FALSE(material.Transparent);
    EXPECT_FLOAT_EQ(material.Alpha, 1.0f);
}

TEST(MaterialTest, StateModification)
{
    MaterialInstance material;
    
    material.AlbedoPath = "textures/test_albedo.png";
    material.OverrideAlbedo = true;
    
    material.Metalness = 0.8f;
    material.Roughness = 0.2f;
    material.Transparent = true;
    material.Alpha = 0.5f;

    EXPECT_TRUE(material.OverrideAlbedo);
    EXPECT_EQ(material.AlbedoPath, "textures/test_albedo.png");
    EXPECT_FLOAT_EQ(material.Metalness, 0.8f);
    EXPECT_FLOAT_EQ(material.Roughness, 0.2f);
    EXPECT_TRUE(material.Transparent);
    EXPECT_FLOAT_EQ(material.Alpha, 0.5f);
}
