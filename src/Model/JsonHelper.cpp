#include "JsonHelper.h"
#include <Color/ColorParser.h>
#include <raylib.h>

// Safe value retrieval
std::optional<std::string> JsonHelper::GetString(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_string())
    {
        return j[key].get<std::string>();
    }
    return std::nullopt;
}

std::optional<float> JsonHelper::GetFloat(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_number())
    {
        return j[key].get<float>();
    }
    return std::nullopt;
}

std::optional<bool> JsonHelper::GetBool(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_boolean())
    {
        return j[key].get<bool>();
    }
    return std::nullopt;
}

std::optional<int> JsonHelper::GetInt(const json &j, const std::string &key)
{
    if (j.contains(key) && j[key].is_number_integer())
    {
        return j[key].get<int>();
    }
    return std::nullopt;
}

// Complex type parsing
Vector3 JsonHelper::ParseVector3(const json &j, const Vector3 &defaultValue)
{
    if (!j.is_object())
        return defaultValue;

    return {j.value("x", defaultValue.x), j.value("y", defaultValue.y),
            j.value("z", defaultValue.z)};
}

Color JsonHelper::ParseColor(const json &j, const Color &defaultValue)
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
bool JsonHelper::ValidateModelEntry(const json &entry)
{
    return HasRequiredKeys(entry, {"name", "path"});
}

bool JsonHelper::ValidateInstanceEntry(const json &entry)
{
    // Instance may not have mandatory fields
    return entry.is_object();
}

bool JsonHelper::HasRequiredKeys(const json &j, const std::vector<std::string> &keys)
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
ModelFileConfig JsonHelper::ParseModelConfig(const json &entry)
{
    ModelFileConfig config;

    config.name = entry["name"].get<std::string>();
    config.path = entry["path"].get<std::string>();
    config.category = entry.value("category", "default");
    config.spawn = entry.value("spawn", true);
    config.hasCollision = entry.value("hasCollision", false);

    // Parse collision precision
    std::string precisionStr = entry.value("collisionPrecision", "auto");
    if (precisionStr == "auto" || precisionStr == "automatic")
        config.collisionPrecision = CollisionPrecision::AUTO;
    else if (precisionStr == "aabb" || precisionStr == "simple")
        config.collisionPrecision = CollisionPrecision::AABB_ONLY;
    else if (precisionStr == "octree" || precisionStr == "octree_only")
        config.collisionPrecision = CollisionPrecision::OCTREE_ONLY;
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
            config.instances.push_back(ParseInstanceConfig(instance));
        }
    }

    return config;
}

ModelInstanceConfig JsonHelper::ParseInstanceConfig(const json &entry)
{
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