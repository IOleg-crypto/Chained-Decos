#include "engine/scene/serialization_utils.h"
#include "gtest/gtest.h"
#include <filesystem>

using namespace CHEngine;
using namespace CHEngine::SerializationUtils;

// Simple struct for nested serialization testing
struct NestedData
{
    float X = 0.0f;
    int Y = 0;
};

void SerializeNested(YAML::Emitter& out, const NestedData& data)
{
    out << YAML::BeginMap;
    out << YAML::Key << "X" << YAML::Value << data.X;
    out << YAML::Key << "Y" << YAML::Value << data.Y;
    out << YAML::EndMap;
}

void DeserializeNested(NestedData& data, YAML::Node node)
{
    if (node["X"]) data.X = node["X"].as<float>();
    if (node["Y"]) data.Y = node["Y"].as<int>();
}

TEST(SerializationTest, PropertyArchiveBasic)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    {
        PropertyArchive archive(out);
        float f = 1.23f;
        int i = 42;
        std::string s = "hello";
        
        archive.Property("Float", f)
               .Property("Int", i)
               .Property("String", s);
    }
    out << YAML::EndMap;

    YAML::Node node = YAML::Load(out.c_str());
    EXPECT_FLOAT_EQ(node["Float"].as<float>(), 1.23f);
    EXPECT_EQ(node["Int"].as<int>(), 42);
    EXPECT_EQ(node["String"].as<std::string>(), "hello");

    // Deserialize
    PropertyArchive in(node);
    float f2 = 0;
    int i2 = 0;
    std::string s2 = "";
    in.Property("Float", f2)
      .Property("Int", i2)
      .Property("String", s2);

    EXPECT_FLOAT_EQ(f2, 1.23f);
    EXPECT_EQ(i2, 42);
    EXPECT_EQ(s2, "hello");
}

TEST(SerializationTest, PropertyArchiveHandle)
{
    UUID id;
    YAML::Emitter out;
    out << YAML::BeginMap;
    {
        PropertyArchive archive(out);
        archive.Handle("MyID", id);
    }
    out << YAML::EndMap;

    YAML::Node node = YAML::Load(out.c_str());
    EXPECT_EQ(node["MyID"].as<uint64_t>(), (uint64_t)id);

    UUID id2(0);
    PropertyArchive in(node);
    in.Handle("MyID", id2);
    EXPECT_EQ((uint64_t)id2, (uint64_t)id);
}

TEST(SerializationTest, PropertyArchiveNested)
{
    NestedData data = { 3.14f, 7 };
    YAML::Emitter out;
    out << YAML::BeginMap;
    {
        PropertyArchive archive(out);
        archive.Nested("Settings", data, SerializeNested, DeserializeNested);
    }
    out << YAML::EndMap;

    YAML::Node node = YAML::Load(out.c_str());
    EXPECT_TRUE(node["Settings"].IsMap());
    EXPECT_FLOAT_EQ(node["Settings"]["X"].as<float>(), 3.14f);
    EXPECT_EQ(node["Settings"]["Y"].as<int>(), 7);

    NestedData data2;
    PropertyArchive in(node);
    in.Nested("Settings", data2, SerializeNested, DeserializeNested);
    EXPECT_FLOAT_EQ(data2.X, 3.14f);
    EXPECT_EQ(data2.Y, 7);
}
