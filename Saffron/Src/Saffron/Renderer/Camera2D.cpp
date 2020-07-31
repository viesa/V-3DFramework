﻿#include "Saffron/SaffronPCH.h"

#include "Saffron/Renderer/Camera2D.h"

namespace Se
{

Camera2D::Camera2D(float left, float right, float bottom, float top)
	:
	m_ProjectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)),
	m_ViewMatrix(1.0f),
	m_ViewProjectionMatrix(m_ProjectionMatrix *m_ViewMatrix)
{
	//SE_PROFILE_FUNCTION();
}

void Camera2D::SetProjection(float left, float right, float bottom, float top)
{
	//SE_PROFILE_FUNCTION();

	m_ProjectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void Camera2D::RecalculateViewMatrix()
{
	//SE_PROFILE_FUNCTION();

	const glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
		glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

	m_ViewMatrix = glm::inverse(transform);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

}