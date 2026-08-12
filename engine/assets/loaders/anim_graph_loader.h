#ifndef CH_ANIM_GRAPH_LOADER_H
#define CH_ANIM_GRAPH_LOADER_H

#include "engine/assets/loaders/iasset_loader.h"
#include "engine/assets/types/animation_graph_asset.h"
#include <memory>
#include <string>

namespace Chained
{
	class AnimGraphLoader : public IAssetLoader
	{
	public:
		bool IsAsync() const override
		{
			return false;
		}
		std::shared_ptr<Asset> Create() override;
		bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath,
				  std::string* outError = nullptr) override;

		bool Save(const AnimationGraphAsset& graph, const std::string& path);
	};
} // namespace Chained

#endif // CH_ANIM_GRAPH_LOADER_H
