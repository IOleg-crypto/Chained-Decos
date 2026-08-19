#ifndef CH_TEXTURE_LOADER_H
#define CH_TEXTURE_LOADER_H

#include "engine/assets/loaders/iasset_loader.h"
#include "engine/assets/types/texture_asset.h"
#include <memory>
#include <string>

namespace Chained
{
	class TextureLoader : public IAssetLoader
	{
	public:
		bool IsAsync() const override
		{
			return true;
		}
		std::shared_ptr<Asset> Create() override;
		bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath,
				  std::string* outError = nullptr) override;
	};
} // namespace Chained

#endif // CH_TEXTURE_LOADER_H
