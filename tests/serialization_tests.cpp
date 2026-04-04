#include "engine/scene/serialization_utils.h"
#include "gtest/gtest.h"
#include <filesystem>

using namespace CHEngine;
using namespace CHEngine::SerializationUtils;

struct NestedData
{
    float X = 0.0f;
    int Y = 0;

    template<typename Archive>
    void Reflect(CHEngine::Properties<Archive>& props)
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
        float f = 1.23f;
        int i = 42;
        std::string s = "hello";

        archive.Property("Float", f);
        archive.Property("Int", i);
        archive.Property("String", s);
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
    in.Property("Float", f2);
    in.Property("Int", i2);
    in.Property("String", s2);

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
    YAML::Emitter out;
    PropertyArchive archive(out);
    
    NestedData data;
    data.X = 10.5f;
    data.Y = 42;
    
    archive.Nested("Settings", data);
    
    YAML::Node inNode = YAML::Load(out.c_str());
    PropertyArchive in(inNode);
    
    NestedData data2;
}
