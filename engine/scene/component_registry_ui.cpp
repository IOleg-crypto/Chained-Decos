// UI component registrations (Control, UIAction)
// Split into its own TU to reduce obj file size in MinGW Clang Debug builds.
// Note: UIControlComponent is massive and registered in ui_control_registry.cpp.
#include "component_registry.h"
#include "components/ui/control_component.h"
#include "components/ui/ui_action_component.h"

namespace Chained
{
	void RegisterUIComponents()
	{
		ComponentRegistry::RegisterReflective<ControlComponent>("Control", nullptr, "UI");
		ComponentRegistry::RegisterReflective<UIActionComponent>("UI Action", nullptr, "UI");
	}
} // namespace Chained
