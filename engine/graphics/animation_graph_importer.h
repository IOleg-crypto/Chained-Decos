#ifndef CH_ANIMATION_GRAPH_IMPORTER_H
#define CH_ANIMATION_GRAPH_IMPORTER_H


#include <string>
#include <memory>

namespace CHEngine
{
    class AnimationGraphImporter
    {
    public:
        static std::shared_ptr<AnimationGraphAsset> Import(const std::string& path);
        static bool Save(const std::shared_ptr<AnimationGraphAsset>& graph, const std::string& path);
    };
}

#endif // CH_ANIMATION_GRAPH_IMPORTER_H
