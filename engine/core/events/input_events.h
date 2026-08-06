#ifndef CH_INPUT_EVENTS_H
#define CH_INPUT_EVENTS_H
#include "engine/core/key_codes.h"
#include "engine/core/events/events.h"

namespace Chained
{
	class KeyPressedEvent : public Event
	{
	public:
		KeyPressedEvent(KeyCode keycode, bool isRepeat = false)
			: m_KeyCode(keycode),
			  m_IsRepeat(isRepeat)
		{
		}
		KeyCode GetKeyCode() const
		{
			return m_KeyCode;
		}
		bool IsRepeat() const
		{
			return m_IsRepeat;
		}
		EVENT_CLASS_TYPE(KeyPressed)
		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	private:
		KeyCode m_KeyCode;
		bool m_IsRepeat;
	};
} // namespace Chained
#endif // CH_INPUT_EVENTS_H
