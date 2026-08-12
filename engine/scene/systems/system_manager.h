#ifndef CH_SYSTEM_MANAGER_H
#define CH_SYSTEM_MANAGER_H

#include "engine/common/base.h"
#include "engine/common/timestep.h"
#include <functional>
#include <vector>

namespace Chained
{

	class Scene;

	class CH_API SystemManager
	{
	public:
		enum class Phase : uint8_t
		{
			PreUpdate = 0,
			Update = 1,
			PostUpdate = 2
		};

		using SystemFn = std::function<void(Scene&, Timestep)>;

		struct SystemEntry
		{
			Phase phase;
			SystemFn function;
			bool enabledInEditor;
			bool enabledInRuntime;
			bool enabledInSimulation;
		};

	public:
		void RegisterSystem(Phase phase, SystemFn fn, bool editor = true, bool runtime = true, bool simulation = true);

		void UpdateEditor(Scene& scene, Timestep ts);
		void UpdateRuntime(Scene& scene, Timestep ts);
		void UpdateSimulation(Scene& scene, Timestep ts);

	private:
		void ExecutePhase(Scene& scene, Timestep ts, Phase phase, bool editor, bool runtime, bool simulation);

		std::vector<SystemEntry> m_Systems;
	};

} // namespace Chained

#endif // CH_SYSTEM_MANAGER_H
