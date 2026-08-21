#ifndef CH_FONT_LOADER_H
#define CH_FONT_LOADER_H

#include "engine/assets/loaders/iasset_loader.h"
#include "engine/assets/asset.h"
#include <memory>
#include <string>

namespace Chained
{
	class FontLoader : public IAssetLoader
	{
	public:
		bool IsAsync() const override
		{
			return false;
		}
		std::shared_ptr<Asset> Create() override;
		bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath,
				  std::string* outError = nullptr) override;
	};
} // namespace Chained

#endif // CH_FONT_LOADER_H
