#include "engine/core/service_locator.h"
#include "engine/core/service_registry.h"

namespace Chained
{
	ServiceRegistry& ServiceLocator::GetRegistry()
	{
		static ServiceRegistry registry;
		return registry;
	}

	CH_API void ServiceLocator::Lock()
	{
		GetRegistry().Lock();
	}

	CH_API void ServiceLocator::InitializeModule()
	{
		GetRegistry().InitializeModules();
	}

	CH_API void ServiceLocator::Shutdown()
	{
		GetRegistry().Shutdown();
	}

	CH_API bool ServiceLocator::IsAvailable()
	{
		return GetRegistry().IsAvailable();
	}

} // namespace Chained