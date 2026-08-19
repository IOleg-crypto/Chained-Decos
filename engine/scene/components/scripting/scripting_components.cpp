#include "engine/scene/components/scripting/scripting_components.h"
#include "engine/core/service_locator.h"
#include "engine/reflection/reflection.h"
#include "engine/scripting/scriptengine.h"

namespace Chained
{

	template <typename T_Archive> void ManagedScriptInstance::Reflect(::Chained::Properties<T_Archive>& props)
	{
		if (props.GetMode() == ::Chained::ReflectionMode::UI)
		{
			std::vector<std::string> options;
			options.emplace_back("-- Select script --");
			auto* scriptEngine = ::Chained::ServiceLocator::TryGet<::Chained::ScriptEngine>();
			if (scriptEngine)
			{
				for (const auto& [scriptName, scriptType] : scriptEngine->GetScriptClasses())
				{
					options.emplace_back(scriptName);
				}
			}
			std::sort(options.begin() + 1, options.end());
			props.StringEnum("ClassName", ClassName, options);
		}
		else
		{
			props.Property("ClassName", ClassName);
		}
	}

	template void ManagedScriptInstance::Reflect<IPropertyArchiveBase>(Properties<IPropertyArchiveBase>&);

} // namespace Chained
