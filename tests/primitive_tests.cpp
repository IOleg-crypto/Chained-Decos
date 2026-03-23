// primitive_tests.cpp
// Tests for the PrimitiveComponent ECS component and procedural mesh generation via MeshImporter.
// The ProceduralModelGeneration test requires a raylib window and is skipped on CI.
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/component_serializer.h"
#include "engine/graphics/importers/mesh_importer.h"
#include "gtest/gtest.h"

using namespace CHEngine;

class PrimitiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!IsWindowReady())
        {
            SetConfigFlags(FLAG_WINDOW_HIDDEN);
            InitWindow(1, 1, "PrimitiveTest");
        }
    }

    void TearDown() override {}
};

// Verifies that a default-constructed PrimitiveComponent has sensible defaults
// (Type=None, Radius=0.5, Height=1, Slices/Stacks=16, not dirty, no asset).
TEST_F(PrimitiveTest, Defaults)
{
    PrimitiveComponent comp;
    EXPECT_EQ(comp.Type, PrimitiveType::None);
    EXPECT_FLOAT_EQ(comp.Radius, 0.5f);
    EXPECT_FLOAT_EQ(comp.Height, 1.0f);
    EXPECT_EQ(comp.Slices, 16);
    EXPECT_EQ(comp.Stacks, 16);
    EXPECT_FALSE(comp.Dirty);
    EXPECT_EQ(comp.Asset, nullptr);
}

// Verifies that a PrimitiveComponent can be serialized to YAML and deserialized back
// into a new entity, preserving all fields including Type, Radius, Slices, and Stacks.
// The deserialized component must also be marked Dirty so the mesh is re-generated.
TEST_F(PrimitiveTest, Serialization)
{
    auto& serializer = ComponentSerializer::Get();

    Scene scene;
    Entity entity = scene.CreateEntity("PrimitiveEntity");
    auto& primitive = entity.AddComponent<PrimitiveComponent>();
    primitive.Type   = PrimitiveType::Sphere;
    primitive.Radius = 1.5f;
    primitive.Slices = 32;
    primitive.Stacks = 24;

    // Serialize to YAML string
    YAML::Emitter out;
    out << YAML::BeginMap;
    serializer.SerializeAll(out, entity);
    out << YAML::EndMap;

    // Deserialize into a new entity in the same scene
    Entity other = scene.CreateEntity("DeserializedPrimitive");
    YAML::Node data = YAML::Load(out.c_str());
    serializer.DeserializeAll(other, data);

    EXPECT_TRUE(other.HasComponent<PrimitiveComponent>());
    auto& otherPrim = other.GetComponent<PrimitiveComponent>();
    EXPECT_EQ(otherPrim.Type, PrimitiveType::Sphere);
    EXPECT_FLOAT_EQ(otherPrim.Radius, 1.5f);
    EXPECT_EQ(otherPrim.Slices, 32);
    EXPECT_EQ(otherPrim.Stacks, 24);
    EXPECT_TRUE(otherPrim.Dirty); // Must be dirty so mesh is re-generated on first frame
}

// Verifies that MeshImporter::GenerateProceduralModel produces valid meshes for
// built-in :sphere: and :cube: keys. Requires a raylib context.
TEST_F(PrimitiveTest, ProceduralModelGeneration)
{
#if defined(CH_CI)
    GTEST_SKIP() << "Skipping procedural generation test on CI (no OpenGL).";
#endif

    if (!IsWindowReady())
    {
        GTEST_SKIP() << "Skipping procedural generation test: Raylib window/context not ready.";
    }

    ProceduralParameters params;
    params.Radius = 1.0f;
    params.Slices = 20;
    
    // :sphere: — should return a model with at least 1 mesh and > 0 vertices
    Model sphere = MeshImporter::GenerateProceduralModel(":sphere:", params);
    EXPECT_GT(sphere.meshCount, 0);
    if (sphere.meshCount > 0)
    {
        EXPECT_GT(sphere.meshes[0].vertexCount, 0);
        UnloadModel(sphere);
    }

    // :cube: — same expectations, different shape
    params.Dimensions = {2.0f, 3.0f, 4.0f};
    Model cube = MeshImporter::GenerateProceduralModel(":cube:", params);
    EXPECT_GT(cube.meshCount, 0);
    if (cube.meshCount > 0)
    {
        EXPECT_GT(cube.meshes[0].vertexCount, 0);
        UnloadModel(cube);
    }
}
