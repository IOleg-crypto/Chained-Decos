#include "engine/assets/loaders/mesh_merger.h"

#include "engine/core/profiler.h"
#include <cfloat>

namespace Chained
{

	void MeshMerger::Process(const std::filesystem::path& modelPath, std::vector<MeshData>& meshes,
							 std::vector<MaterialData>& /*materials*/, std::vector<MeshInstance>& instances)
	{
		CH_PROFILE_FUNCTION();
		if (instances.empty() || meshes.empty())
		{
			return;
		}

		try
		{
			struct InstanceGroup
			{
				std::vector<int> instanceIndices;
			};
			std::unordered_map<int, InstanceGroup> groups;
			for (int i = 0; i < (int)instances.size(); ++i)
			{
				int meshIdx = instances[i].meshIndex;
				if (meshIdx >= 0 && meshIdx < (int)meshes.size())
				{
					groups[meshes[meshIdx].materialIndex].instanceIndices.push_back(i);
				}
			}

			CH_CORE_INFO("MeshMerger: Merging {} instances into {} material groups", instances.size(), groups.size());

			std::vector<MeshData> mergedMeshes;
			std::vector<MeshInstance> mergedInstances;
			mergedMeshes.reserve(groups.size());
			mergedInstances.reserve(groups.size());

			for (auto& [matIdx, group] : groups)
			{
				MeshData merged;
				merged.materialIndex = matIdx;

				size_t totalVertices = 0;
				size_t totalIndices = 0;
				for (int instIdx : group.instanceIndices)
				{
					int srcIdx = instances[instIdx].meshIndex;
					totalVertices += meshes[srcIdx].vertices.size();
					totalIndices += meshes[srcIdx].indices.size();
				}

				merged.vertices.reserve(totalVertices);
				merged.texcoords.reserve(totalVertices / 3 * 2);
				merged.normals.reserve(totalVertices);
				merged.tangents.reserve(totalVertices);
				merged.colors.reserve(totalVertices / 3 * 4);
				merged.indices.reserve(totalIndices);

				int firstMeshIdx = instances[group.instanceIndices[0]].meshIndex;
				bool hasSkins = !meshes[firstMeshIdx].joints.empty();
				if (hasSkins)
				{
					merged.joints.reserve(totalVertices / 3 * 4);
					merged.weights.reserve(totalVertices / 3 * 4);
				}

				merged.MinBounds = {FLT_MAX, FLT_MAX, FLT_MAX};
				merged.MaxBounds = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

				for (int instIdx : group.instanceIndices)
				{
					const auto& inst = instances[instIdx];
					const auto& src = meshes[inst.meshIndex];
					uint32_t vertexOffset = (uint32_t)(merged.vertices.size() / 3);

					glm::mat4 t = inst.localTransform;
					glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(t)));

					merged.texcoords.insert(merged.texcoords.end(), src.texcoords.begin(), src.texcoords.end());
					merged.colors.insert(merged.colors.end(), src.colors.begin(), src.colors.end());

					const size_t vertCount = src.vertices.size() / 3;
					const bool hasNormals = src.normals.size() == src.vertices.size();
					const bool hasTangents = src.tangents.size() == src.vertices.size();

					if (!hasSkins)
					{
						merged.vertices.reserve(merged.vertices.size() + src.vertices.size());
						if (hasNormals)
						{
							merged.normals.reserve(merged.normals.size() + src.normals.size());
						}
						if (hasTangents)
						{
							merged.tangents.reserve(merged.tangents.size() + src.tangents.size());
						}

						for (size_t v = 0, n = 0, tg = 0; v < src.vertices.size(); v += 3, n += 3, tg += 3)
						{
							glm::vec4 pos =
								t * glm::vec4(src.vertices[v], src.vertices[v + 1], src.vertices[v + 2], 1.0f);
							merged.vertices.push_back(pos.x);
							merged.vertices.push_back(pos.y);
							merged.vertices.push_back(pos.z);

							if (hasNormals)
							{
								glm::vec3 norm = glm::normalize(
									normalMatrix * glm::vec3(src.normals[n], src.normals[n + 1], src.normals[n + 2]));
								merged.normals.push_back(norm.x);
								merged.normals.push_back(norm.y);
								merged.normals.push_back(norm.z);
							}

							if (hasTangents)
							{
								glm::vec3 tan =
									glm::normalize(normalMatrix * glm::vec3(src.tangents[tg], src.tangents[tg + 1],
																			src.tangents[tg + 2]));
								merged.tangents.push_back(tan.x);
								merged.tangents.push_back(tan.y);
								merged.tangents.push_back(tan.z);
							}
						}
					}
					else
					{
						merged.vertices.reserve(merged.vertices.size() + src.vertices.size());
						if (hasNormals)
						{
							merged.normals.reserve(merged.normals.size() + src.normals.size());
						}
						if (hasTangents)
						{
							merged.tangents.reserve(merged.tangents.size() + src.tangents.size());
						}

						for (size_t v = 0, n = 0, tg = 0; v < src.vertices.size(); v += 3, n += 3, tg += 3)
						{
							merged.vertices.push_back(src.vertices[v]);
							merged.vertices.push_back(src.vertices[v + 1]);
							merged.vertices.push_back(src.vertices[v + 2]);

							if (hasNormals)
							{
								merged.normals.push_back(src.normals[n]);
								merged.normals.push_back(src.normals[n + 1]);
								merged.normals.push_back(src.normals[n + 2]);
							}

							if (hasTangents)
							{
								merged.tangents.push_back(src.tangents[tg]);
								merged.tangents.push_back(src.tangents[tg + 1]);
								merged.tangents.push_back(src.tangents[tg + 2]);
							}
						}
					}

					for (uint32_t idx : src.indices)
					{
						merged.indices.push_back(idx + vertexOffset);
					}

					if (hasSkins)
					{
						if (!src.joints.empty())
						{
							merged.joints.insert(merged.joints.end(), src.joints.begin(), src.joints.end());
							merged.weights.insert(merged.weights.end(), src.weights.begin(), src.weights.end());
						}
						else
						{
							size_t padSize = (src.vertices.size() / 3) * 4;
							merged.joints.insert(merged.joints.end(), padSize, (unsigned char)0);
							merged.weights.insert(merged.weights.end(), padSize, 0.0f);
						}
					}

					if (!hasSkins)
					{
						glm::vec3 corners[8] = {{src.MinBounds.x, src.MinBounds.y, src.MinBounds.z},
												{src.MaxBounds.x, src.MinBounds.y, src.MinBounds.z},
												{src.MinBounds.x, src.MaxBounds.y, src.MinBounds.z},
												{src.MaxBounds.x, src.MaxBounds.y, src.MinBounds.z},
												{src.MinBounds.x, src.MinBounds.y, src.MaxBounds.z},
												{src.MaxBounds.x, src.MinBounds.y, src.MaxBounds.z},
												{src.MinBounds.x, src.MaxBounds.y, src.MaxBounds.z},
												{src.MaxBounds.x, src.MaxBounds.y, src.MaxBounds.z}};
						for (auto& c : corners)
						{
							glm::vec3 tp = glm::vec3(t * glm::vec4(c, 1.0f));
							merged.MinBounds = glm::min(merged.MinBounds, tp);
							merged.MaxBounds = glm::max(merged.MaxBounds, tp);
						}
					}
					else
					{
						for (size_t v = 0; v < src.vertices.size(); v += 3)
						{
							glm::vec3 p = {src.vertices[v], src.vertices[v + 1], src.vertices[v + 2]};
							merged.MinBounds = glm::min(merged.MinBounds, p);
							merged.MaxBounds = glm::max(merged.MaxBounds, p);
						}
					}
				}

				int newMeshIdx = (int)mergedMeshes.size();
				mergedMeshes.push_back(std::move(merged));
				mergedInstances.push_back({newMeshIdx, glm::mat4(1.0f)});
			}

			meshes = std::move(mergedMeshes);
			instances = std::move(mergedInstances);
		} catch (const std::bad_alloc& e)
		{
			CH_CORE_ERROR("MeshMerger: Out of memory during mesh merge for '{}', keeping unmerged meshes: {}",
						  modelPath.string(), e.what());
		}
	}

} // namespace Chained
