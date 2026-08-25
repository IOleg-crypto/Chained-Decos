#include "camera.h"
#include "editor/layer.h"
#include "engine/app/application.h"
#include "engine/core/input.h"
#include "engine/scene/components/render/camera_component.h"
#include "engine/scene/systems/transform_system.h"
#include "engine/scene/components/core/transform_component.h"
#include "imgui.h"

#include <algorithm>
#include <glm/gtx/quaternion.hpp>

namespace Chained
{
	static constexpr float kMouseSensitivity = 0.003f;

	// Pan speed: viewport-independent polynomial curve (empirically tuned)
	static constexpr float kPanSpeedDivisor = 1000.0f;
	static constexpr float kPanSpeedCap = 2.4f;
	static constexpr float kPanSpeedA = 0.0366f;
	static constexpr float kPanSpeedB = -0.1778f;
	static constexpr float kPanSpeedC = 0.3021f;

	// Zoom speed: quadratic distance-based curve
	static constexpr float kZoomDistanceScale = 0.2f;
	static constexpr float kZoomSpeedMin = 0.1f;
	static constexpr float kZoomSpeedMax = 100.0f;

	EditorCameraController::EditorCameraController()
	{
		SetPerspective(glm::radians(45.0f), 0.1f, 10000.0f);
		UpdateView();
	}

	Camera3D EditorCameraController::ToCamera3D() const
	{
		Camera3D camera;
		glm::vec3 pos = CalculatePosition();
		glm::vec3 fp = m_FocalPoint;
		glm::vec3 up = GetUpDirection();
		camera.Position = {pos.x, pos.y, pos.z};
		camera.Target = {fp.x, fp.y, fp.z};
		camera.Up = {up.x, up.y, up.z};
		camera.Projection = GetProjectionType();
		camera.FovDegrees = m_FovDegrees;
		camera.OrthographicSize = GetOrthographicSize();
		camera.NearClip = m_NearClip;
		camera.FarClip = m_FarClip;
		camera.ViewMatrix = m_ViewMatrix;
		camera.ProjectionMatrix = GetProjection();
		return camera;
	}

	void EditorCameraController::Set2DMode(bool enabled)
	{
		if (m_Is2DMode == enabled)
		{
			return;
		}

		m_Is2DMode = enabled;

		if (m_Is2DMode)
		{
			m_SavedPitch = m_Pitch;
			m_SavedYaw = m_Yaw;
			m_SavedProjectionType = GetProjectionType();

			m_Pitch = 0.0f;
			m_Yaw = 0.0f;
			SetProjectionType(ProjectionType::Orthographic);
		}
		else
		{
			m_Pitch = m_SavedPitch;
			m_Yaw = m_SavedYaw;
			SetProjectionType(m_SavedProjectionType);
		}
		UpdateView();
	}

	void EditorCameraController::OnUpdate(Entity cameraEntity, Timestep ts, const glm::vec2& viewportSize)
	{
		m_ViewportWidth = (uint32_t)viewportSize.x;
		m_ViewportHeight = (uint32_t)viewportSize.y;

		float deltaTime = ts;
		float moveSpeed = m_MoveSpeed;
		float boostMultiplier = m_BoostMultiplier;

		bool hasImGui = ImGui::GetCurrentContext() != nullptr;

		bool rightDown = hasImGui ? ImGui::IsMouseDown(ImGuiMouseButton_Right)
								  : Core::Input::IsMouseButtonDown(MouseCode::ButtonRight);
		bool middleDown = hasImGui ? ImGui::IsMouseDown(ImGuiMouseButton_Middle)
								   : Core::Input::IsMouseButtonDown(MouseCode::ButtonMiddle);
		bool leftDown = hasImGui ? ImGui::IsMouseDown(ImGuiMouseButton_Left)
								 : Core::Input::IsMouseButtonDown(MouseCode::ButtonLeft);

		bool shiftDown =
			hasImGui ? (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))
					 : (Core::Input::IsKeyDown(KeyCode::LeftShift) || Core::Input::IsKeyDown(KeyCode::RightShift));
		bool altDown = hasImGui
						   ? (ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt))
						   : (Core::Input::IsKeyDown(KeyCode::LeftAlt) || Core::Input::IsKeyDown(KeyCode::RightAlt));

		auto isKeyDown = [hasImGui](KeyCode coreKey, ImGuiKey imguiKey) -> bool {
			if (hasImGui)
			{
				return ImGui::IsKeyDown(imguiKey);
			}
			return Core::Input::IsKeyDown(coreKey);
		};

		bool hasEntity = cameraEntity && cameraEntity.HasComponent<TransformComponent>() &&
						 cameraEntity.HasComponent<CameraComponent>();

		// In Play mode: sync editor camera FROM TransformComponent (e.g. scripts or inspector changes).
		// In Edit mode: skip — the editor camera is authoritative, TransformComponent write-back happens below.
		if (hasEntity && EditorLayer::Get().GetSceneState() == SceneState::Play && !rightDown && !middleDown)
		{
			auto& tc = cameraEntity.GetComponent<TransformComponent>();
			if (std::isfinite(tc.Rotation.x) && std::isfinite(tc.Rotation.y))
			{
				if (fabsf(tc.Rotation.x - m_Pitch) > 0.01f || fabsf(tc.Rotation.y - m_Yaw) > 0.01f)
				{
					m_Pitch = tc.Rotation.x;
					m_Yaw = tc.Rotation.y;
					UpdateView();
				}
			}
		}

		glm::vec2 delta = {0.0f, 0.0f};
		if (hasImGui)
		{
			ImVec2 imguiDelta = ImGui::GetIO().MouseDelta;
			delta = {imguiDelta.x * kMouseSensitivity, imguiDelta.y * kMouseSensitivity};
		}
		else
		{
			delta = Core::Input::GetMouseDelta() * kMouseSensitivity;
		}

		if (rightDown)
		{
			MouseRotate(delta);

			float speed = moveSpeed * deltaTime;
			if (shiftDown)
			{
				speed *= boostMultiplier;
			}

			glm::vec3 fwd = GetForwardDirection();
			glm::vec3 rgt = GetRightDirection();
			glm::vec3 upg = {0, 1, 0};

			glm::vec3 currentPos = CalculatePosition();

			if (isKeyDown(KeyCode::W, ImGuiKey_W))
			{
				currentPos += (m_Is2DMode ? upg : fwd) * speed;
			}
			if (isKeyDown(KeyCode::S, ImGuiKey_S))
			{
				currentPos -= (m_Is2DMode ? upg : fwd) * speed;
			}
			if (isKeyDown(KeyCode::D, ImGuiKey_D))
			{
				currentPos += rgt * speed;
			}
			if (isKeyDown(KeyCode::A, ImGuiKey_A))
			{
				currentPos -= rgt * speed;
			}
			if (isKeyDown(KeyCode::E, ImGuiKey_E))
			{
				currentPos += upg * speed;
			}
			if (isKeyDown(KeyCode::Q, ImGuiKey_Q))
			{
				currentPos -= upg * speed;
			}

			m_FocalPoint = currentPos + (fwd * m_Distance);
			UpdateView();
		}

		if (middleDown)
		{
			if (shiftDown)
			{
				MousePan(delta);
			}
			else
			{
				MouseRotate(delta);
			}
		}

		if (altDown && leftDown)
		{
			MouseRotate(delta);
		}

		float wheel = hasImGui ? ImGui::GetIO().MouseWheel : Core::Input::GetMouseWheelMove();
		if (wheel != 0.0f && !m_DisableZoom)
		{
			MouseZoom(wheel);
		}

		// Write back rotation and position to the entity's TransformComponent in both
		// Play and Edit modes so the inspector stays in sync with the editor camera.
		if (hasEntity)
		{
			auto& tc = cameraEntity.GetComponent<TransformComponent>();
			TransformSystem::SetRotation(tc, glm::vec3(m_Pitch, m_Yaw, 0.0f));
			TransformSystem::SetTranslation(tc, CalculatePosition());
		}
	}

	void EditorCameraController::UpdateView()
	{
		glm::vec3 position = CalculatePosition();
		glm::quat orientation = GetOrientation();
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	void EditorCameraController::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
		UpdateView();
	}

	void EditorCameraController::MouseRotate(const glm::vec2& delta)
	{
		if (m_Is2DMode)
		{
			return;
		}

		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
		UpdateView();
	}

	void EditorCameraController::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 0.1f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 0.1f;
		}
		UpdateView();
	}

	glm::vec3 EditorCameraController::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	glm::vec3 EditorCameraController::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}
	glm::vec3 EditorCameraController::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}
	glm::vec3 EditorCameraController::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}
	glm::quat EditorCameraController::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

	std::pair<float, float> EditorCameraController::PanSpeed() const
	{
		float x = std::min((float)m_ViewportWidth / kPanSpeedDivisor, kPanSpeedCap);
		float xFactor = kPanSpeedA * (x * x) + kPanSpeedB * x + kPanSpeedC;
		float y = std::min((float)m_ViewportHeight / kPanSpeedDivisor, kPanSpeedCap);
		float yFactor = kPanSpeedA * (y * y) + kPanSpeedB * y + kPanSpeedC;
		return {xFactor, yFactor};
	}

	float EditorCameraController::RotationSpeed() const
	{
		return m_RotationSpeed;
	}

	float EditorCameraController::ZoomSpeed() const
	{
		float distance = m_Distance * kZoomDistanceScale;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;

		return std::clamp(speed * m_ZoomSpeedMultiplier, kZoomSpeedMin, kZoomSpeedMax);
	}

} // namespace Chained