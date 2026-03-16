#ifndef CH_SETTINGS_MANAGER_H
#define CH_SETTINGS_MANAGER_H

#include <string>
#include <filesystem>
#include <map>
#include <variant>

namespace CHEngine
{
class SettingsManager
{
public:
    using SettingValue = std::variant<bool, int, float, std::string>;

    SettingsManager(const std::filesystem::path& path);
    ~SettingsManager() = default;

    void Set(const std::string& group, const std::string& key, const SettingValue& value);
    
    template<typename T>
    T Get(const std::string& group, const std::string& key, const T& defaultValue) const
    {
        auto groupIt = m_Settings.find(group);
        if (groupIt != m_Settings.end())
        {
            auto keyIt = groupIt->second.find(key);
            if (keyIt != groupIt->second.end())
            {
                if (std::holds_alternative<T>(keyIt->second))
                {
                    return std::get<T>(keyIt->second);
                }
                
                // Special case: float vs int
                if constexpr (std::is_same_v<T, float>)
                {
                    if (std::holds_alternative<int>(keyIt->second))
                        return (float)std::get<int>(keyIt->second);
                }
            }
        }
        return defaultValue;
    }

    bool Load();
    bool Save() const;

private:
    std::filesystem::path m_Path;
    std::map<std::string, std::map<std::string, SettingValue>> m_Settings;
};
} // namespace CHEngine

#endif // CH_SETTINGS_MANAGER_H
