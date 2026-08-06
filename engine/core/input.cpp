#include "input.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"

namespace Chained::Core
{
	Input* Input::s_Instance = nullptr;

	Input::Input()
	{
		s_Instance = this;
	}

	Input::~Input()
	{
		if (s_Instance == this)
		{
			s_Instance = nullptr;
		}
	}

	void Input::Initialize()
	{
		ResetAll();
	}

	void Input::Shutdown()
	{
		ResetAll();
	}

	void Input::ResetAll()
	{
		if (!s_Instance)
		{
			return;
		}
		s_Instance->m_KeyStates.fill(false);
		s_Instance->m_LastKeyStates.fill(false);
		s_Instance->m_MouseStates.fill(false);
		s_Instance->m_LastMouseStates.fill(false);
		s_Instance->m_MousePosition = {0.0f, 0.0f};
		s_Instance->m_LastMousePosition = {0.0f, 0.0f};
		s_Instance->m_MouseWheelAccumulator = 0.0f;
		s_Instance->m_CurrentMouseWheelDelta = 0.0f;
		s_Instance->m_FirstMouseUpdate = true;
	}

	void Input::Update(Timestep ts)
	{
		if (!s_Instance)
		{
			return;
		}
		s_Instance->m_LastKeyStates = s_Instance->m_KeyStates;
		s_Instance->m_LastMouseStates = s_Instance->m_MouseStates;
		s_Instance->m_LastMousePosition = s_Instance->m_MousePosition;
		s_Instance->m_CurrentMouseWheelDelta = s_Instance->m_MouseWheelAccumulator;
		s_Instance->m_MouseWheelAccumulator = 0.0f;
	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		if (!s_Instance)
		{
			return false;
		}
		auto code = static_cast<size_t>(key);
		if (code >= s_Instance->m_KeyStates.size())
		{
			return false;
		}
		return s_Instance->m_KeyStates[code] && !s_Instance->m_LastKeyStates[code];
	}

	bool Input::IsKeyDown(KeyCode key)
	{
		if (!s_Instance)
		{
			return false;
		}
		auto code = static_cast<size_t>(key);
		if (code >= s_Instance->m_KeyStates.size())
		{
			return false;
		}
		return s_Instance->m_KeyStates[code];
	}

	bool Input::IsKeyReleased(KeyCode key)
	{
		if (!s_Instance)
		{
			return false;
		}
		auto code = static_cast<size_t>(key);
		if (code >= s_Instance->m_KeyStates.size())
		{
			return false;
		}
		return !s_Instance->m_KeyStates[code] && s_Instance->m_LastKeyStates[code];
	}

	bool Input::IsKeyUp(KeyCode key)
	{
		if (!s_Instance)
		{
			return true;
		}
		auto code = static_cast<size_t>(key);
		if (code >= s_Instance->m_KeyStates.size())
		{
			return true;
		}
		return !s_Instance->m_KeyStates[code];
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		if (!s_Instance)
		{
			return false;
		}
		auto code = static_cast<size_t>(button);
		if (code >= s_Instance->m_MouseStates.size())
		{
			return false;
		}
		return s_Instance->m_MouseStates[code] && !s_Instance->m_LastMouseStates[code];
	}

	bool Input::IsMouseButtonDown(MouseCode button)
	{
		if (!s_Instance)
		{
			return false;
		}
		auto code = static_cast<size_t>(button);
		if (code >= s_Instance->m_MouseStates.size())
		{
			return false;
		}
		return s_Instance->m_MouseStates[code];
	}

	bool Input::IsMouseButtonReleased(MouseCode button)
	{
		if (!s_Instance)
		{
			return false;
		}
		auto code = static_cast<size_t>(button);
		if (code >= s_Instance->m_MouseStates.size())
		{
			return false;
		}
		return !s_Instance->m_MouseStates[code] && s_Instance->m_LastMouseStates[code];
	}

	bool Input::IsMouseButtonUp(MouseCode button)
	{
		if (!s_Instance)
		{
			return true;
		}
		auto code = static_cast<size_t>(button);
		if (code >= s_Instance->m_MouseStates.size())
		{
			return true;
		}
		return !s_Instance->m_MouseStates[code];
	}

	glm::vec2 Input::GetMousePosition()
	{
		return s_Instance ? s_Instance->m_MousePosition : glm::vec2{0.0f, 0.0f};
	}

	glm::vec2 Input::GetMouseDelta()
	{
		if (!s_Instance)
		{
			return {0.0f, 0.0f};
		}
		if (s_Instance->m_FirstMouseUpdate)
		{
			return {0.0f, 0.0f};
		}
		return s_Instance->m_MousePosition - s_Instance->m_LastMousePosition;
	}

	float Input::GetMouseWheelMove()
	{
		return s_Instance ? s_Instance->m_CurrentMouseWheelDelta : 0.0f;
	}

	void Input::OnKey(KeyCode key, bool pressed)
	{
		if (!s_Instance)
		{
			return;
		}
		auto code = static_cast<size_t>(key);
		if (code < s_Instance->m_KeyStates.size())
		{
			s_Instance->m_KeyStates[code] = pressed;
		}
	}

	void Input::OnMouseButton(MouseCode button, bool pressed)
	{
		if (!s_Instance)
		{
			return;
		}
		auto code = static_cast<size_t>(button);
		if (code < s_Instance->m_MouseStates.size())
		{
			s_Instance->m_MouseStates[code] = pressed;
		}
	}

	void Input::OnMouseMove(float x, float y)
	{
		if (!s_Instance)
		{
			return;
		}
		if (s_Instance->m_FirstMouseUpdate)
		{
			s_Instance->m_LastMousePosition = {x, y};
			s_Instance->m_FirstMouseUpdate = false;
		}
		s_Instance->m_MousePosition = {x, y};
	}

	void Input::OnMouseScroll(float xOffset, float yOffset)
	{
		if (!s_Instance)
		{
			return;
		}
		s_Instance->m_MouseWheelAccumulator += yOffset;
	}

} // namespace Chained::Core
