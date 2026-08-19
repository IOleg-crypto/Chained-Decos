#include "engine/scene/components/render/primitive_component.h"
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
	EXPECT_TRUE(component.MeshPath.empty());
}

TEST(PrimitiveTest, TypeConstructorSetsRequestedPrimitiveType)
{
	PrimitiveComponent sphere;
	sphere.Type = PrimitiveType::Sphere;

	EXPECT_EQ(sphere.Type, PrimitiveType::Sphere);
	EXPECT_FLOAT_EQ(sphere.Radius, 0.5f);
	EXPECT_EQ(sphere.Slices, 16);
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
	source.MeshPath = "primitives/TestCube.chmesh";

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
	EXPECT_EQ(copy.MeshPath, source.MeshPath);
}

#include "engine/scene/scene.h"
#include "engine/scene/components/render/model_component.h"

TEST(PrimitiveTest, PrimitiveSystemBakesMeshAndAttachesModelComponent)
{
	auto scene = Scene::CreateDefault();
	Entity entity = scene->CreateEntity("TestCube");

	PrimitiveComponent prim;
	prim.Type = PrimitiveType::Cube;
	prim.Dimensions = {2.0f, 2.0f, 2.0f};
	entity.AddComponent<PrimitiveComponent>(prim);

	// PrimitiveSystem should have baked the mesh and attached ModelComponent
	EXPECT_TRUE(entity.HasComponent<ModelComponent>());
	auto& mc = entity.GetComponent<ModelComponent>();
	EXPECT_EQ(mc.ModelPath, "primitives/TestCube.chmesh");

	auto& primComp = entity.GetComponent<PrimitiveComponent>();
	EXPECT_EQ(primComp.MeshPath, mc.ModelPath);

	// Removing PrimitiveComponent removes ModelComponent
	entity.RemoveComponent<PrimitiveComponent>();
	EXPECT_FALSE(entity.HasComponent<ModelComponent>());
}
