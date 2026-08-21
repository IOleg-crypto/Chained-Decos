#ifndef CH_SHADER_LOADER_H
#define CH_SHADER_LOADER_H

#include "engine/assets/loaders/iasset_loader.h"
#include "engine/assets/types/shader_asset.h"
#include <memory>
#include <string>
#include <vector>

namespace Chained
{
	class ShaderLoader : public IAssetLoader
	{
	public:
		bool IsAsync() const override
		{
			return false;
		}
		std::shared_ptr<Asset> Create() override;
		bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath,
				  std::string* outError = nullptr) override;

		std::shared_ptr<Shader> LoadShaderFromPath(const std::string& path);
		std::shared_ptr<Shader> LoadShaderFromPaths(const std::string& vsPath, const std::string& fsPath);
		std::string ProcessShaderSource(const std::string& path, std::vector<std::string>& includedFiles);
	};
} // namespace Chained

#endif // CH_SHADER_LOADER_H
