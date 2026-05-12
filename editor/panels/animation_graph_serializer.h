#pragma once

#include "engine/scene/animation_graph_data.h"
#include <filesystem>

namespace CHEngine {

class AnimationGraphSerializer {
public:
    static void Save(const std::filesystem::path& path, const AnimationGraphData& data);
    static bool Load(const std::filesystem::path& path, AnimationGraphData& outData);
};

} // namespace CHEngine
