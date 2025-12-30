#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "ModelConfig.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <version>

using json = nlohmann::json;

// Parser namespace for safe JSON parsing
namespace JsonParser
{
// Safe JSON value retrieval
std::optional<std::string> GetString(const json &j, const std::string &key);
std::optional<float> GetFloat(const json &j, const std::string &key);
std::optional<bool> GetBool(const json &j, const std::string &key);
std::optional<int> GetInt(const json &j, const std::string &key);

// Complex type parsing
Vector3 ParseVector3(const json &j, const Vector3 &defaultValue = {0, 0, 0});
Color ParseColor(const json &j, const Color &defaultValue = WHITE);

// JSON structure validation
bool ValidateModelEntry(const json &entry);
bool ValidateInstanceEntry(const json &entry);

// Configuration parsing
std::optional<ModelFileConfig> ParseModelConfig(const json &entry);
std::optional<ModelInstanceConfig> ParseInstanceConfig(const json &entry);
} // namespace JsonParser

#endif // JSON_PARSER_H
