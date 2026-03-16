#include "settings_manager.h"
#include "yaml-cpp/yaml.h"
#include "engine/core/log.h"
#include <fstream>

namespace CHEngine
{
SettingsManager::SettingsManager(const std::filesystem::path& path)
    : m_Path(path)
{
}

void SettingsManager::Set(const std::string& group, const std::string& key, const SettingValue& value)
{
    m_Settings[group][key] = value;
}

bool SettingsManager::Load()
{
    if (!std::filesystem::exists(m_Path))
        return false;

    try
    {
        YAML::Node data = YAML::LoadFile(m_Path.string());
        for (auto groupNode : data)
        {
            if (!groupNode.first.IsScalar()) continue;
            
            std::string group = groupNode.first.as<std::string>();
            if (!groupNode.second.IsMap()) continue;

            for (auto kvNode : groupNode.second)
            {
                if (!kvNode.first.IsScalar()) continue;
                
                std::string key = kvNode.first.as<std::string>();
                auto valNode = kvNode.second;
                
                if (valNode.IsScalar())
                {
                    // Attempt to deduce type from YAML node
                    // yaml-cpp's Tag() or logical deduction
                    try {
                        // First try bool
                        if (valNode.Tag() == "tag:yaml.org,2002:bool" || valNode.as<std::string>() == "true" || valNode.as<std::string>() == "false")
                            m_Settings[group][key] = valNode.as<bool>();
                        else if (valNode.Tag() == "tag:yaml.org,2002:int")
                            m_Settings[group][key] = valNode.as<int>();
                        else if (valNode.Tag() == "tag:yaml.org,2002:float")
                            m_Settings[group][key] = valNode.as<float>();
                        else
                        {
                            // Fallback to manual check if tags are missing
                            std::string s = valNode.as<std::string>();
                            if (s.find_first_not_of("0123456789-") == std::string::npos)
                                m_Settings[group][key] = valNode.as<int>();
                            else if (s.find_first_not_of("0123456789.-") == std::string::npos && s.find('.') != std::string::npos)
                                m_Settings[group][key] = valNode.as<float>();
                            else
                                m_Settings[group][key] = s;
                        }
                    } catch (...) {
                        m_Settings[group][key] = valNode.as<std::string>();
                    }
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("Failed to load settings from {0}: {1}", m_Path.string(), e.what());
        return false;
    }
}

bool SettingsManager::Save() const
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    for (auto const& [group, keys] : m_Settings)
    {
        out << YAML::Key << group << YAML::Value << YAML::BeginMap;
        for (auto const& [key, value] : keys)
        {
            out << YAML::Key << key << YAML::Value;
            std::visit([&out](auto&& arg) { out << arg; }, value);
        }
        out << YAML::EndMap;
    }
    out << YAML::EndMap;

    std::ofstream fout(m_Path);
    if (!fout.is_open())
        return false;
        
    fout << out.c_str();
    return true;
}

} // namespace CHEngine
