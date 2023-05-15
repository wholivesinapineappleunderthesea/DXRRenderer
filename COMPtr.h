#pragma once

#include "W32Platform.h"
#include <Unknwn.h>

template <typename T> struct COMPtrOut
{
	inline COMPtrOut() = delete;
	inline COMPtrOut(const COMPtrOut&) = delete;
	inline COMPtrOut(COMPtrOut&&) = delete;

	inline COMPtrOut(T** ptr) noexcept : m_pptr(ptr)
	{
	}

	inline ~COMPtrOut()
	{
	}

	inline operator T**() const noexcept
	{
		return m_pptr;
	}
	inline operator void**() const noexcept
	{
		return reinterpret_cast<void**>(m_pptr);
	}

  private:
	T** m_pptr{};
};

template <typename T> struct COMPtrInOut
{
	inline COMPtrInOut() = delete;
	inline COMPtrInOut(const COMPtrInOut&) = delete;
	inline COMPtrInOut(COMPtrInOut&&) = delete;

	inline COMPtrInOut(T** ptr) noexcept : m_pptr(ptr), m_original(*ptr)
	{
	}

	inline ~COMPtrInOut()
	{
		if (m_original && m_original != *m_pptr)
		{
			m_original->Release();
		}
	}

	inline operator T**() const noexcept
	{
		return m_pptr;
	}
	inline operator void**() const noexcept
	{
		return reinterpret_cast<void**>(m_pptr);
	}

  private:
	T** m_pptr{};
	IUnknown* m_original{};
};

template <typename T> struct COMPtr
{
	static inline auto static_uuid = __uuidof(T);

	using type = T;
	using pointer = T*;
	using reference = T&;

	inline COMPtr() noexcept : m_ptr(nullptr)
	{
	}

	inline COMPtr(std::nullptr_t) noexcept : m_ptr(nullptr)
	{
	}

	// This constructor assumes that the pointer already has a reference.
	inline COMPtr(pointer ptr) noexcept : m_ptr(ptr)
	{
	}

	// This constructor assumes that the pointer already has a reference.
	inline COMPtr(void* ptr) noexcept : m_ptr(reinterpret_cast<pointer>(ptr))
	{
	}

	inline COMPtr(const COMPtr<T>& other) noexcept : m_ptr(other.m_ptr)
	{
		AddRef();
	}

	inline COMPtr(COMPtr<T>&& other) noexcept : m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}

	inline auto operator=(const COMPtr<T>& other) noexcept -> COMPtr<T>&
	{
		Release();
		m_ptr = other.m_ptr;
		AddRef();
		return *this;
	}

	inline auto operator=(COMPtr<T>&& other) noexcept -> COMPtr<T>&
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
		return *this;
	}

	inline ~COMPtr()
	{
		Release();
	}

	inline auto operator->() const noexcept -> pointer
	{
		return m_ptr;
	}

	inline auto Get() const noexcept -> pointer
	{
		return m_ptr;
	}

	inline auto Reset(T* ptr = nullptr) noexcept -> void
	{
		Release();
		m_ptr = ptr;
	}

	inline auto Release() noexcept -> void
	{
		if (m_ptr)
		{
			m_ptr->Release();
			m_ptr = nullptr;
		}
	}

	inline operator bool() const noexcept
	{
		return m_ptr != nullptr;
	}

	inline bool operator!() const noexcept
	{
		return m_ptr == nullptr;
	}

	inline auto operator==(pointer p) const noexcept -> bool

	{
		return m_ptr == p;
	}

	inline auto operator!=(pointer p) const noexcept -> bool
	{
		return m_ptr != p;
	}

	inline auto operator==(std::nullptr_t) const noexcept -> bool
	{
		return m_ptr == nullptr;
	}

	inline auto operator!=(std::nullptr_t) const noexcept -> bool
	{
		return m_ptr != nullptr;
	}

	inline auto operator==(const COMPtr<T>& other) const noexcept -> bool
	{
		return m_ptr == other.m_ptr;
	}

	inline auto operator!=(const COMPtr<T>& other) const noexcept -> bool

	{
		return m_ptr != other.m_ptr;
	}

	// Very similar to std::inout_ptr()
	inline auto InOut() noexcept -> COMPtrInOut<T>
	{
		return &m_ptr;
	}

	// Very similar to std::out_ptr()
	inline auto Out() noexcept -> COMPtrOut<T>
	{
		return &m_ptr;
	}

  private:
	pointer m_ptr = nullptr;

	inline auto AddRef() noexcept -> void
	{
		if (m_ptr)
		{
			m_ptr->AddRef();
		}
	}
};