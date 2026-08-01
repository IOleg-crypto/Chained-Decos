#ifndef CH_MESH_MERGER_H
#define CH_MESH_MERGER_H

#include "engine/assets/model_data.h"
#include <vector>
#include <string>
#include <filesystem>

namespace Chained
{

class MeshMerger
{
public:
    static void Process(const std::filesystem::path& modelPath, std::vector<MeshData>& meshes,
                        std::vector<MaterialData>& materials, std::vector<MeshInstance>& instances);
};

} // namespace Chained

#endif // CH_MESH_MERGER_H
