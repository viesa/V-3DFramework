#include "Saffron/SaffronPCH.h"

#include "Saffron/Editor/EditorCamera.h"

namespace Se
{
EditorCamera::EditorCamera(const glm::mat4 &projectionMatrix)
	: Camera(projectionMatrix)
{
	m_Rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_FocalPoint = glm::vec3(0.0f);

	glm::vec3 position = { -5, 5, 5 };
	m_Distance = glm::distance(position, m_FocalPoint);

	m_Yaw = 3.0f * Math::PI / 4.0f;
	m_Pitch = Math::PI / 4.0f;

	UpdateCameraView();
}

void EditorCamera::Focus()
{
}

void EditorCamera::OnUpdate(const Keyboard &keyboard, const Mouse &mouse, Time ts)
{
	if ( keyboard.IsPressed(SE_KEY_LEFT_ALT) )
	{
		const glm::vec2 delta = (mouse.GetPosition() - m_InitialMousePosition) * 0.003f;
		m_InitialMousePosition = mouse.GetPosition();

		if ( mouse.IsPressed(SE_BUTTON_MIDDLE) )
			OnMousePan(delta);
		else if ( mouse.IsPressed(SE_BUTTON_LEFT) )
			OnMouseRotate(delta);
		else if ( mouse.IsPressed(SE_BUTTON_RIGHT) )
			OnMouseZoom(delta.y);
	}

	UpdateCameraView();
}

void EditorCamera::OnEvent(const Event &event)
{
	const EventDispatcher dispatcher(event);
	dispatcher.Try<MouseScrollEvent>(SE_EVENT_FN(EditorCamera::OnMouseScroll));
}

glm::vec3 EditorCamera::GetUpDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 EditorCamera::GetRightDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 EditorCamera::GetForwardDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::quat EditorCamera::GetOrientation() const
{
	return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
}

void EditorCamera::UpdateCameraView()
{
	m_Position = CalculatePosition();

	const glm::quat orientation = GetOrientation();
	m_Rotation = glm::eulerAngles(orientation) * (180.0f / Math::PI);
	m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
	m_ViewMatrix = glm::inverse(m_ViewMatrix);
}

void EditorCamera::OnMouseScroll(const MouseScrollEvent &event)
{
	const float delta = event.GetOffsetY() * 0.1f;
	OnMouseZoom(delta);
	UpdateCameraView();
}

void EditorCamera::OnMousePan(const glm::vec2 &delta)
{
	const auto &speed = GetPanSpeed();
	m_FocalPoint += -GetRightDirection() * delta.x * speed.x * m_Distance;
	m_FocalPoint += GetUpDirection() * delta.y * speed.y * m_Distance;
}

void EditorCamera::OnMouseRotate(const glm::vec2 &delta)
{
	const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
	m_Yaw += yawSign * delta.x * GetRotationSpeed();
	m_Pitch += delta.y * GetRotationSpeed();
}

void EditorCamera::OnMouseZoom(float delta)
{
	m_Distance -= delta * GetZoomSpeed();
	if ( m_Distance < 1.0f )
	{
		m_FocalPoint += GetForwardDirection();
		m_Distance = 1.0f;
	}
}

glm::vec3 EditorCamera::CalculatePosition()
{
	return m_FocalPoint - GetForwardDirection() * m_Distance;
}

glm::vec2 EditorCamera::GetPanSpeed() const
{
	const float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
	const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

	const float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
	const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

	return { xFactor, yFactor };
}

float EditorCamera::GetRotationSpeed() const
{
	return 0.8f;
}

float EditorCamera::GetZoomSpeed() const
{
	float distance = m_Distance * 0.2f;
	distance = std::max(distance, 0.0f);
	float speed = distance * distance;
	speed = std::min(speed, 100.0f); // max speed = 100
	return speed;
}
}