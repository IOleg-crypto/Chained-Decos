#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/component_serializer.h"
#include "engine/graphics/mesh_importer.h"
#include "gtest/gtest.h"

using namespace CHEngine;

class PrimitiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // HIDDEN window for raylib resource loading tests
        if (!IsWindowReady())
        {
            SetConfigFlags(FLAG_WINDOW_HIDDEN);
            InitWindow(1, 1, "PrimitiveTest");
        }
    }

    void TearDown() override
    {
        // Don't close window here as it might be used by other tests in the same process
        // unless we are sure we want to re-init it every time.
        // For simplicity, let's just keep it open or use a wrapper.
    }
};

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

TEST_F(PrimitiveTest, Serialization)
{
    auto& serializer = ComponentSerializer::Get();

    Scene scene;
    Entity entity = scene.CreateEntity("PrimitiveEntity");
    auto& primitive = entity.AddComponent<PrimitiveComponent>();
    primitive.Type = PrimitiveType::Sphere;
    primitive.Radius = 1.5f;
    primitive.Slices = 32;
    primitive.Stacks = 24;

    // Serialize
    YAML::Emitter out;
    out << YAML::BeginMap;
    serializer.SerializeAll(out, entity);
    out << YAML::EndMap;

    // Deserialize into another entity
    Entity other = scene.CreateEntity("DeserializedPrimitive");
    YAML::Node data = YAML::Load(out.c_str());
    serializer.DeserializeAll(other, data);

    EXPECT_TRUE(other.HasComponent<PrimitiveComponent>());
    auto& otherPrim = other.GetComponent<PrimitiveComponent>();
    EXPECT_EQ(otherPrim.Type, PrimitiveType::Sphere);
    EXPECT_FLOAT_EQ(otherPrim.Radius, 1.5f);
    EXPECT_EQ(otherPrim.Slices, 32);
    EXPECT_EQ(otherPrim.Stacks, 24);
    EXPECT_TRUE(otherPrim.Dirty); // Should be dirty after deserialization
}

TEST_F(PrimitiveTest, ProceduralModelGeneration)
{
    // Test that generation doesn't crash and returns meshes
    // Note: We can't easily check mesh content without a GPU context in some environments,
    // but MeshImporter::GenerateProceduralModel uses Raylib's GenMeshXXX which are CPU-side.

    ProceduralParameters params;
    params.Radius = 1.0f;
    params.Slices = 20;
    
    // Sphere
    Model sphere = MeshImporter::GenerateProceduralModel(":sphere:", params);
    EXPECT_GT(sphere.meshCount, 0);
    // Raylib's GenMeshSphere(radius, rings, slices)
    // Mesh count should be 1
    if (sphere.meshCount > 0)
    {
        EXPECT_GT(sphere.meshes[0].vertexCount, 0);
        UnloadModel(sphere);
    }

    // Cube
    params.Dimensions = {2.0f, 3.0f, 4.0f};
    Model cube = MeshImporter::GenerateProceduralModel(":cube:", params);
    EXPECT_GT(cube.meshCount, 0);
    if (cube.meshCount > 0)
    {
        EXPECT_GT(cube.meshes[0].vertexCount, 0);
        UnloadModel(cube);
    }
}
