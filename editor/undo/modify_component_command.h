#ifndef CH_MODIFY_COMPONENT_COMMAND_H
#define CH_MODIFY_COMPONENT_COMMAND_H

#include "command.h"

#include "engine/scene/scene.h"
#include <string>

namespace Chained
{

	template <typename T> class ModifyComponentCommand : public IEditorCommand
	{
	public:
		ModifyComponentCommand(Entity entity, const T& oldState, const T& newState, const std::string& name = "")
			: m_Entity(entity),
			  m_OldState(oldState),
			  m_NewState(newState),
			  m_Name(name)
		{
		}

		void Execute() override
		{
			if (m_Entity.IsValid() && m_Entity.HasComponent<T>())
			{
				m_Entity.GetComponent<T>() = m_NewState;
			}
		}

		void Undo() override
		{
			if (m_Entity.IsValid() && m_Entity.HasComponent<T>())
			{
				m_Entity.GetComponent<T>() = m_OldState;
			}
		}

		std::string GetName() const override
		{
			return m_Name.empty() ? "Modify Component" : m_Name;
		}

	private:
		Entity m_Entity;
		T m_OldState;
		T m_NewState;
		std::string m_Name;
	};

} // namespace Chained

#endif // CH_MODIFY_COMPONENT_COMMAND_H
