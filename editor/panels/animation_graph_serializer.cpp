#include "animation_graph_serializer.h"
#include "engine/core/log.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace CHEngine {

void AnimationGraphSerializer::Save(const std::filesystem::path& path, const AnimationGraphData& data)
{
    std::filesystem::create_directories(path.parent_path());
    
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Nodes" << YAML::Value << YAML::BeginSeq;
    for (const auto& node : data.Nodes)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << node.Name;
        out << YAML::Key << "Clip" << YAML::Value << node.AnimationPath;
        out << YAML::Key << "Loop" << YAML::Value << node.IsLooping;
        out << YAML::Key << "X" << YAML::Value << node.Rect.Min.x;
        out << YAML::Key << "Y" << YAML::Value << node.Rect.Min.y;
        out << YAML::Key << "Transitions" << YAML::Value << YAML::BeginSeq;
        for (const auto& t : node.Transitions) out << t;
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "Links" << YAML::Value << YAML::BeginSeq;
    for (const auto& link : data.Links)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "From" << YAML::Value << link.FromState;
        out << YAML::Key << "FromSlot" << YAML::Value << link.FromSlot;
        out << YAML::Key << "To" << YAML::Value << link.ToState;
        out << YAML::Key << "ToSlot" << YAML::Value << link.ToSlot;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(path);
    fout << out.c_str();
    CH_CORE_INFO("Animation Graph saved to {}", path.string());
}

bool AnimationGraphSerializer::Load(const std::filesystem::path& path, AnimationGraphData& outData)
{
    if (!std::filesystem::exists(path)) return false;

    try {
        YAML::Node data = YAML::LoadFile(path.string());
        outData.Nodes.clear();
        outData.Links.clear();

        if (data["Nodes"])
        {
            for (auto node : data["Nodes"])
            {
                AnimationState sn;
                sn.Name = node["Name"].as<std::string>();
                sn.AnimationPath = node["Clip"].as<std::string>();
                sn.IsLooping = node["Loop"].as<bool>();
                float x = node["X"].as<float>();
                float y = node["Y"].as<float>();
                sn.Rect = ImRect(x, y, x + 250, y + 220);
                if (node["Transitions"])
                    for (auto t : node["Transitions"]) sn.Transitions.push_back(t.as<std::string>());
                outData.Nodes.push_back(sn);
            }
        }
        if (data["Links"])
        {
            for (auto link : data["Links"])
                outData.Links.push_back({ link["From"].as<size_t>(), link["FromSlot"].as<size_t>(), link["To"].as<size_t>(), link["ToSlot"].as<size_t>() });
        }
        CH_CORE_INFO("Animation Graph loaded from {}", path.string());
        return true;
    } catch (const std::exception& e) {
        CH_CORE_ERROR("Failed to load Animation Graph: {}", e.what());
        return false;
    }
}

} // namespace CHEngine
