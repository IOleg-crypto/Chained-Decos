#include "engine/scene/components/primitive_component.h"
#include "engine/scene/components/primitive_runtime.h"
#include "gtest/gtest.h"

using namespace Chained;

TEST(PrimitiveTest, Defaults)
{
    PrimitiveComponent component;

    EXPECT_EQ(component.Type, PrimitiveType::None);
    EXPECT_FLOAT_EQ(component.Radius, 0.5f);
    EXPECT_FLOAT_EQ(component.InnerRadius, 0.2f);
    EXPECT_FLOAT_EQ(component.Height, 1.0f);
    EXPECT_EQ(component.Slices, 16);
    EXPECT_EQ(component.Stacks, 16);
    EXPECT_FLOAT_EQ(component.Dimensions.x, 1.0f);
    EXPECT_FLOAT_EQ(component.Dimensions.y, 1.0f);
    EXPECT_FLOAT_EQ(component.Dimensions.z, 1.0f);

    // Runtime state is a separate component; check its defaults independently.
    PrimitiveRuntimeState rt;
    EXPECT_TRUE(rt.Dirty); // defaults to true so mesh is built on first frame
    EXPECT_EQ(rt.Asset, nullptr);
}

TEST(PrimitiveTest, TypeConstructorSetsRequestedPrimitiveType)
{
    PrimitiveComponent sphere;
    sphere.Type = PrimitiveType::Sphere;

    EXPECT_EQ(sphere.Type, PrimitiveType::Sphere);
    EXPECT_FLOAT_EQ(sphere.Radius, 0.5f);
    EXPECT_EQ(sphere.Slices, 16);
    // Dirty flag is no longer on PrimitiveComponent -- it lives in PrimitiveRuntimeState.
}

TEST(PrimitiveTest, CopyConstructorPreservesConfiguredValues)
{
    PrimitiveComponent source;
    source.Type = PrimitiveType::Cylinder;
    source.Radius = 2.5f;
    source.InnerRadius = 0.7f;
    source.Height = 4.0f;
    source.Slices = 24;
    source.Stacks = 12;
    source.Dimensions = {2.0f, 3.0f, 4.0f};

    PrimitiveComponent copy(source);

    EXPECT_EQ(copy.Type, source.Type);
    EXPECT_FLOAT_EQ(copy.Radius, source.Radius);
    EXPECT_FLOAT_EQ(copy.InnerRadius, source.InnerRadius);
    EXPECT_FLOAT_EQ(copy.Height, source.Height);
    EXPECT_EQ(copy.Slices, source.Slices);
    EXPECT_EQ(copy.Stacks, source.Stacks);
    EXPECT_FLOAT_EQ(copy.Dimensions.x, source.Dimensions.x);
    EXPECT_FLOAT_EQ(copy.Dimensions.y, source.Dimensions.y);
    EXPECT_FLOAT_EQ(copy.Dimensions.z, source.Dimensions.z);
    // Dirty and Asset are runtime-only (PrimitiveRuntimeState) and not part of PrimitiveComponent copy.
}
