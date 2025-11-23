#include "JsonParser.h"
#include "src/Engine/Color/Parser/ColorParser.h"
#include <raylib.h>

// Safe value retrieval
std::optional<std::string> JsonParser::GetString(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_string())
    {
        return j[key].get<std::string>();
    }
    return std::nullopt;
}

std::optional<float> JsonParser::GetFloat(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_number())
    {
        return j[key].get<float>();
    }
    return std::nullopt;
}

std::optional<bool> JsonParser::GetBool(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_boolean())
    {
        return j[key].get<bool>();
    }
    return std::nullopt;
}

std::optional<int> JsonParser::GetInt(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_number_integer())
    {
        return j[key].get<int>();
    }
    return std::nullopt;
}

// Complex type parsing
Vector3 JsonParser::ParseVector3(const json &j, const Vector3 &defaultValue)
{
    if (!j.is_object())
        return defaultValue;

    return {j.value("x", defaultValue.x), j.value("y", defaultValue.y),
            j.value("z", defaultValue.z)};
}

Color JsonParser::ParseColor(const json &j, const Color &defaultValue)
{
    if (j.is_string())
    {
        return ParseColorByName(j.get<std::string>());
    }
    else if (j.is_object())
    {
        return {static_cast<unsigned char>(j.value("r", 255)),
                static_cast<unsigned char>(j.value("g", 255)),
                static_cast<unsigned char>(j.value("b", 255)),
                static_cast<unsigned char>(j.value("a", 255))};
    }
    return defaultValue;
}

// Validation
bool JsonParser::ValidateModelEntry(const json &entry)
{
    return HasRequiredKeys(entry, {"name", "path"});
}

bool JsonParser::ValidateInstanceEntry(const json &entry)
{
    // Instance may not have mandatory fields
    return entry.is_object();
}

bool JsonParser::HasRequiredKeys(const json &j, const std::vector<std::string> &keys)
{
    for (const auto &key : keys)
    {
        if (!j.contains(key))
        {
            return false;
        }
    }
    return true;
}

// Configuration parsing
std::optional<ModelFileConfig> JsonParser::ParseModelConfig(const json &entry)
{
    if (!ValidateModelEntry(entry))
    {
        return std::nullopt;
    }

    ModelFileConfig config;

    auto name = GetString(entry, "name");
    auto path = GetString(entry, "path");
    
    if (!name || !path)
    {
        return std::nullopt;
    }

    config.name = *name;
    config.path = *path;
    config.category = entry.value("category", "default");
    config.spawn = entry.value("spawn", true);
    config.hasCollision = entry.value("hasCollision", false);

    // Parse collision precision
    std::string precisionStr = entry.value("collisionPrecision", "auto");
    if (precisionStr == "auto" || precisionStr == "automatic")
        config.collisionPrecision = CollisionPrecision::AUTO;
    else if (precisionStr == "aabb" || precisionStr == "simple")
        config.collisionPrecision = CollisionPrecision::AABB_ONLY;
    else if (precisionStr == "bvh" || precisionStr == "bvh_only")
        config.collisionPrecision = CollisionPrecision::BVH_ONLY;
    else if (precisionStr == "improved" || precisionStr == "balanced")
        config.collisionPrecision = CollisionPrecision::IMPROVED_AABB;
    else if (precisionStr == "precise" || precisionStr == "triangle")
        config.collisionPrecision = CollisionPrecision::TRIANGLE_PRECISE;
    else
        config.collisionPrecision = CollisionPrecision::AUTO; // Changed default to AUTO

    config.lodDistance = entry.value("lodDistance", 100.0f);
    config.preload = entry.value("preload", true);
    config.priority = entry.value("priority", 0);

    // Parse instances
    if (entry.contains("instances") && entry["instances"].is_array())
    {
        for (const auto &instance : entry["instances"])
        {
            if (auto instanceConfig = ParseInstanceConfig(instance))
            {
                config.instances.push_back(*instanceConfig);
            }
        }
    }

    return config;
}

std::optional<ModelInstanceConfig> JsonParser::ParseInstanceConfig(const json &entry)
{
    if (!entry.is_object())
    {
        return std::nullopt;
    }

    ModelInstanceConfig config;

    if (entry.contains("position"))
    {
        config.position = ParseVector3(entry["position"]);
    }
    if (entry.contains("rotation"))
    {
        config.rotation = ParseVector3(entry["rotation"]);
    }

    config.scale = entry.value("scale", 1.0f);
    config.spawn = entry.value("spawn", true);
    config.tag = entry.value("tag", "");

    if (entry.contains("color"))
    {
        config.color = ParseColor(entry["color"]);
    }

    return config;
}