#ifndef CH_ASSIMP_IMPORTER_H
#define CH_ASSIMP_IMPORTER_H

#include "engine/assets/loaders/model_loader.h"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct aiScene;
struct aiNode;

namespace Chained
{
	class AssimpImporter
	{
	public:
		static PendingModelData Import(const std::filesystem::path& path, int samplingFPS);

	private:
		AssimpImporter(const std::filesystem::path& path, int samplingFPS, const aiScene* scene);
		~AssimpImporter() = default;

		PendingModelData Execute();

		void ProcessNode(aiNode* node, int parentIdx);
		void ProcessHierarchy();
		void ProcessSingleMesh(uint32_t meshIndex);
		void ProcessMeshes();
		void DecodeEmbeddedTextures();

	private:
		std::filesystem::path m_Path;
		std::filesystem::path m_ModelDir;
		int m_SamplingFPS = 30;
		const aiScene* m_Scene = nullptr;

		PendingModelData m_Data;
		std::unordered_map<std::string, int> m_NameToIndex;
		std::vector<std::vector<std::pair<int, glm::mat4>>> m_MeshOffsetWrites;
	};
} // namespace Chained
#endif
