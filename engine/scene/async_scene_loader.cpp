#include "engine/scene/async_scene_loader.h"
#include "engine/assets/asset_manager.h"
#include "engine/common/thread_pool.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/systems/physics_body_system.h"
#include <chrono>

namespace Chained
{

	bool AsyncSceneLoader::LoadSceneAsync(const std::filesystem::path& path, SceneLoadCallback onComplete)
	{
		if (m_IsLoading)
		{
			CH_CORE_WARN("AsyncSceneLoader: A scene load is already in progress.");
			return false;
		}

		auto* threadPool = ServiceLocator::TryGet<ThreadPool>();
		if (!threadPool)
		{
			CH_CORE_ERROR("AsyncSceneLoader: ThreadPool service unavailable.");
			if (onComplete)
			{
				onComplete(nullptr, "ThreadPool service unavailable.");
			}
			return false;
		}

		m_IsLoading = true;
		m_CurrentPath = path;
		m_OnComplete = std::move(onComplete);
		SetStatus("Loading scene file...", 0.1f);

		std::filesystem::path scenePath = path;

		m_Future = threadPool->Enqueue([this, scenePath]() -> SceneLoadResult {
			SetStatus("Deserializing scene YAML...", 0.25f);

			auto newScene = std::make_shared<Scene>();
			SceneSerializer serializer(newScene.get());

			if (!serializer.Deserialize(scenePath.string()))
			{
				return SceneLoadResult{nullptr, serializer.GetLastError()};
			}

			SetStatus("Pre-building physics shapes & assets...", 0.6f);

			// Pre-build Jolt physics shapes on worker thread if physics service is available
			if (auto* physicsService = ServiceLocator::TryGet<Physics>())
			{
				if (auto* physicsWorld = physicsService->GetWorld())
				{
					auto view = newScene->GetRegistry().view<RigidBodyComponent, ColliderComponent>();
					for (auto entity : view)
					{
						PhysicsBodyDesc desc;
						if (PhysicsBodySystem::BuildBodyDesc(newScene->GetRegistry(), entity, desc))
						{
							if (desc.Shape == ColliderType::Mesh && !desc.CacheKey.empty())
							{
								physicsWorld->PrebuildShape(desc);
							}
						}
					}
				}
			}

			SetStatus("Finalizing staging scene...", 0.9f);
			return SceneLoadResult{newScene, ""};
		});

		return true;
	}

	void AsyncSceneLoader::OnUpdate(Timestep ts)
	{
		if (!m_IsLoading || !m_Future.valid())
		{
			return;
		}

		if (m_Future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			SetStatus("Finalizing GPU buffer uploads...", 0.95f);

			SceneLoadResult result;
			try
			{
				result = m_Future.get();
			} catch (const std::exception& e)
			{
				result.error = std::string("Exception during scene load: ") + e.what();
			}

			// Finalize GPU uploads on main thread
			if (auto* assetManager = ServiceLocator::TryGet<AssetManager>())
			{
				assetManager->FinalizePendingLoads();
			}

			m_IsLoading = false;
			SetStatus("Complete", 1.0f);

			if (m_OnComplete)
			{
				m_OnComplete(result.scene, result.error);
			}
		}
	}

	void AsyncSceneLoader::Cancel()
	{
		if (m_IsLoading)
		{
			m_IsLoading = false;
			SetStatus("Cancelled", 0.0f);
			m_OnComplete = nullptr;
		}
	}

} // namespace Chained
