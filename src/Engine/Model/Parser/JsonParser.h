#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "../Config/ModelConfig.h"
#include <nlohmann/json.hpp>
#include <version>
#include <optional>

using json = nlohmann::json;

// Parser class for safe JSON parsing
class JsonParser
{
public:
    // Safe JSON value retrieval
    static std::optional<std::string> GetString(const json &j, const std::string &key);
    static std::optional<float> GetFloat(const json &j, const std::string &key);
    static std::optional<bool> GetBool(const json &j, const std::string &key);
    static std::optional<int> GetInt(const json &j, const std::string &key);

    // Complex type parsing
    static Vector3 ParseVector3(const json &j, const Vector3 &defaultValue = {0, 0, 0});
    static Color ParseColor(const json &j, const Color &defaultValue = WHITE);

    // JSON structure validation
    static bool ValidateModelEntry(const json &entry);
    static bool ValidateInstanceEntry(const json &entry);

    // Configuration parsing
    static std::optional<ModelFileConfig> ParseModelConfig(const json &entry);
    static std::optional<ModelInstanceConfig> ParseInstanceConfig(const json &entry);

private:
    static bool HasRequiredKeys(const json &j, const std::vector<std::string> &keys);
};

#endif // JSON_PARSER_H

