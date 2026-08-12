#include "engine/scene/systems/system_manager.h"
#include "engine/scene/scene.h"

namespace Chained
{

	void SystemManager::RegisterSystem(Phase phase, SystemFn fn, bool editor, bool runtime, bool simulation)
	{
		m_Systems.push_back({phase, std::move(fn), editor, runtime, simulation});
	}

	void SystemManager::UpdateEditor(Scene& scene, Timestep ts)
	{
		ExecutePhase(scene, ts, Phase::PreUpdate, true, false, false);
		ExecutePhase(scene, ts, Phase::Update, true, false, false);
		ExecutePhase(scene, ts, Phase::PostUpdate, true, false, false);
	}

	void SystemManager::UpdateRuntime(Scene& scene, Timestep ts)
	{
		ExecutePhase(scene, ts, Phase::PreUpdate, false, true, false);
		ExecutePhase(scene, ts, Phase::Update, false, true, false);
		ExecutePhase(scene, ts, Phase::PostUpdate, false, true, false);
	}

	void SystemManager::UpdateSimulation(Scene& scene, Timestep ts)
	{
		ExecutePhase(scene, ts, Phase::PreUpdate, false, false, true);
		ExecutePhase(scene, ts, Phase::Update, false, false, true);
		ExecutePhase(scene, ts, Phase::PostUpdate, false, false, true);
	}

	void SystemManager::ExecutePhase(Scene& scene, Timestep ts, Phase phase, bool editor, bool runtime, bool simulation)
	{
		for (auto& entry : m_Systems)
		{
			if (entry.phase != phase)
			{
				continue;
			}

			if (editor && !entry.enabledInEditor)
			{
				continue;
			}
			if (runtime && !entry.enabledInRuntime)
			{
				continue;
			}
			if (simulation && !entry.enabledInSimulation)
			{
				continue;
			}

			entry.function(scene, ts);
		}
	}

} // namespace Chained
