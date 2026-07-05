#include "engine/serialization/serialization.h"
#include "engine/scene/components/model_component.h"
#include "gtest/gtest.h"
#include <filesystem>

using namespace Chained;
using namespace Chained::Serialization;

struct NestedData
{
    float X = 0.0f;
    int Y = 0;

    template <typename Archive> void Reflect(Chained::Properties<Archive>& props)
    {
        props.Property("X", X);
        props.Property("Y", Y);
    }
};

TEST(SerializationTest, PropertyArchiveBasic)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    {
        PropertyArchive archive(out);
        float floatValue = 1.23f;
        int intValue = 42;
        std::string stringValue = "hello";

        archive.Property("Float", floatValue);
        archive.Property("Int", intValue);
        archive.Property("String", stringValue);
    }
    out << YAML::EndMap;

    YAML::Node node = YAML::Load(out.c_str());
    EXPECT_FLOAT_EQ(node["Float"].as<float>(), 1.23f);
    EXPECT_EQ(node["Int"].as<int>(), 42);
    EXPECT_EQ(node["String"].as<std::string>(), "hello");

    // Deserialize
    PropertyArchive in(node);
    float loadedFloatValue = 0;
    int loadedIntValue = 0;
    std::string loadedStringValue = "";
    in.Property("Float", loadedFloatValue);
    in.Property("Int", loadedIntValue);
    in.Property("String", loadedStringValue);

    EXPECT_FLOAT_EQ(loadedFloatValue, 1.23f);
    EXPECT_EQ(loadedIntValue, 42);
    EXPECT_EQ(loadedStringValue, "hello");
}

TEST(SerializationTest, PropertyArchiveHandle)
{
    UUID id;
    YAML::Emitter out;
    out << YAML::BeginMap;
    {
        PropertyArchive archive(out);
        uint64_t handle = (uint64_t)id;
        archive.Handle("MyID", handle);
    }
    out << YAML::EndMap;

    YAML::Node node = YAML::Load(out.c_str());
    EXPECT_EQ(node["MyID"].as<uint64_t>(), (uint64_t)id);

    uint64_t id2 = 0;
    PropertyArchive in(node);
    in.Handle("MyID", id2);
    EXPECT_EQ(id2, (uint64_t)id);
}

TEST(SerializationTest, PropertyArchiveNested)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    PropertyArchive archive(out);
    Properties<PropertyArchive> props(archive);

    NestedData data;
    data.X = 10.5f;
    data.Y = 42;

    props.Nested("Settings", data);

    YAML::Node inNode = YAML::Load(out.c_str());
    PropertyArchive in(inNode);

    NestedData data2;
    Properties<PropertyArchive> inProps(in);
    inProps.Nested("Settings", data2);

    EXPECT_FLOAT_EQ(data2.X, 10.5f);
    EXPECT_EQ(data2.Y, 42);

    out << YAML::EndMap;
}

