#ifndef CH_ASYNC_SCENE_LOADER_H
#define CH_ASYNC_SCENE_LOADER_H

#include "engine/common/timestep.h"
#include "engine/scene/scene.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>

namespace Chained
{

	class AsyncSceneLoader
	{
	public:
		using SceneLoadCallback = std::function<void(std::shared_ptr<Scene> newScene, const std::string& error)>;

		struct SceneLoadResult
		{
			std::shared_ptr<Scene> scene;
			std::string error;
		};

		AsyncSceneLoader() = default;
		~AsyncSceneLoader() = default;

		/// Starts an asynchronous scene loading task on a worker thread.
		bool LoadSceneAsync(const std::filesystem::path& path, SceneLoadCallback onComplete);

		/// Call every frame on the main thread to check loading completion and finalize GPU assets.
		void OnUpdate(Timestep ts);

		/// Returns true if a scene is currently being loaded asynchronously.
		bool IsLoading() const
		{
			return m_IsLoading;
		}

		/// Returns current loading progress between 0.0f and 1.0f.
		float GetProgress() const
		{
			return m_Progress;
		}

		/// Returns current human-readable status string.
		std::string GetStatus() const
		{
			std::lock_guard<std::mutex> lock(m_StatusMutex);
			return m_Status;
		}

		/// Cancels any in-flight loading operation.
		void Cancel();

	private:
		void SetStatus(const std::string& status, float progress)
		{
			std::lock_guard<std::mutex> lock(m_StatusMutex);
			m_Status = status;
			m_Progress = progress;
		}

	private:
		std::atomic<bool> m_IsLoading{false};
		std::atomic<float> m_Progress{0.0f};
		mutable std::mutex m_StatusMutex;
		std::string m_Status;

		std::filesystem::path m_CurrentPath;
		std::future<SceneLoadResult> m_Future;
		SceneLoadCallback m_OnComplete;
	};

} // namespace Chained

#endif // CH_ASYNC_SCENE_LOADER_H
