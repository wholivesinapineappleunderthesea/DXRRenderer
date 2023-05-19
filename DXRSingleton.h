#pragma once

#include "DXRCommon.h"

template <typename T> struct DXRSingleton : DXRNonCopyable
{
	static inline T* GetInstance()
	{
		return instance;
	}
	template <typename... Args> inline DXRSingleton(Args&&... args)
	{
		instance = new T(std::forward<Args>(args)...);
	}
	inline ~DXRSingleton()
	{
		delete instance;
	}
private:
	static inline T* instance{};
};

auto InitializeSingletonInstances() -> void;