#include "CameraManager.h"

#include <glm/gtc/matrix_transform.hpp>

CameraManager::CameraManager()
{
}

CameraManager::~CameraManager()
{
}

static auto cameraSinTime = 0.f;

auto CameraManager::Update(float dt) -> void
{
	cameraSinTime += dt;

	m_cameraPosition.x = std::sinf(cameraSinTime) * 5.f;
	m_cameraPosition.y = 0.f;
	m_cameraPosition.z = std::cosf(cameraSinTime) * 5.f;

	const auto lookingAt = glm::vec3{};

	m_cameraForward = glm::normalize(lookingAt - m_cameraPosition);
	m_cameraRight = glm::cross(m_cameraForward, glm::vec3{0.f, 1.f, 0.f});
	m_cameraUp = glm::cross(m_cameraRight, m_cameraForward);

	m_projectionMatrix =
		glm::perspectiveRH_ZO(glm::radians(m_verticalFOV), m_cameraAspectRatio,
			m_nearPlane, m_farPlane);
	m_projectionMatrix = glm::transpose(m_projectionMatrix);

	m_viewMatrix = glm::lookAtRH(
		m_cameraPosition, m_cameraPosition + m_cameraForward, m_cameraUp);
	m_viewMatrix = glm::transpose(m_viewMatrix);

	
}


