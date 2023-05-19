#pragma once

#include "DXRCommon.h"

struct CameraManager
{
	static inline auto GetInstance() -> CameraManager*
	{
		return DXRSingleton<CameraManager>::GetInstance();
	}

	CameraManager();
	~CameraManager();
	auto Update(float dt) -> void;

	inline auto GetViewMatrix() const -> glm::mat4
	{
		return m_viewMatrix;
	}

	inline auto GetProjectionMatrix() const -> glm::mat4
	{
		return m_projectionMatrix;
	}

	inline auto GetCameraPosition() const -> glm::vec3
	{
		return m_cameraPosition;
	}

	inline auto GetCameraForward() const -> glm::vec3
	{
		return m_cameraForward;
	}

	inline auto GetCameraRight() const -> glm::vec3
	{
		return m_cameraRight;
	}

	inline auto GetCameraUp() const -> glm::vec3
	{
		return m_cameraUp;
	}

	inline auto GetCameraAspectRatio() const -> float
	{
		return m_cameraAspectRatio;
	}

	inline auto GetHorizontalFOV() const -> float
	{
		return m_horizontalFOV;
	}

	inline auto GetVerticalFOV() const -> float
	{
		return m_verticalFOV;
	}

	inline auto GetNearPlane() const -> float
	{
		return m_nearPlane;
	}

	inline auto GetFarPlane() const -> float
	{
		return m_farPlane;
	}

	inline auto SetAspectRatio(float ratio) -> void
	{
		m_cameraAspectRatio = ratio;
		m_verticalFOV = m_horizontalFOV / m_cameraAspectRatio;
	}

	inline auto SetHorizontalFOV(float fov) -> void
	{
		m_horizontalFOV = fov;
	}

  private:
	THREAD_MARKER(Update) glm::mat4 m_viewMatrix{}, m_projectionMatrix{};
	THREAD_MARKER(Update) glm::vec3 m_cameraPosition{};
	THREAD_MARKER(Update) glm::vec3 m_cameraForward{};
	THREAD_MARKER(Update) glm::vec3 m_cameraRight{};
	THREAD_MARKER(Update) glm::vec3 m_cameraUp{};
	THREAD_MARKER(Update) float m_cameraAspectRatio{16.f / 9.f};
	THREAD_MARKER(Update) float m_horizontalFOV{90.f};
	THREAD_MARKER(Update) float m_verticalFOV{m_horizontalFOV / m_cameraAspectRatio};
	THREAD_MARKER(Update) float m_nearPlane{0.01f};
	THREAD_MARKER(Update) float m_farPlane{1000.f};
};