#pragma once

#include "W32Platform.h"

template <NTNamespace::HANDLE INVALID_VALUE> struct W32Handle
{
	inline W32Handle() noexcept : m_handle(INVALID_VALUE)
	{
	}
	inline W32Handle(decltype(nullptr)) noexcept : m_handle(INVALID_VALUE)
	{
	}
	inline W32Handle(NTNamespace::HANDLE handle) noexcept : m_handle(handle)
	{
	}
	inline ~W32Handle()
	{
		Close();
	}

	inline W32Handle(const W32Handle&) = delete;
	inline auto operator=(const W32Handle&) -> W32Handle& = delete;

	inline W32Handle(W32Handle&& other) noexcept : m_handle(other.m_handle)
	{
		other.m_handle = INVALID_VALUE;
	}
	inline auto operator=(W32Handle&& other) noexcept -> W32Handle&
	{
		Close();
		m_handle = other.m_handle;
		other.m_handle = INVALID_VALUE;
		return *this;
	}
	inline operator NTNamespace::HANDLE() const noexcept
	{
		return m_handle;
	}
	inline auto Get() const noexcept -> NTNamespace::HANDLE
	{
		return m_handle;
	}
	inline auto Close() noexcept -> void
	{
		if (m_handle != INVALID_VALUE)
		{
			NTNamespace::CloseHandle(m_handle);
			m_handle = INVALID_VALUE;
		}
	}

  private:
	NTNamespace::HANDLE m_handle;
};