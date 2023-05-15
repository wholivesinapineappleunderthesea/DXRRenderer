#pragma once

/*
	Windows.h is notorius for being terrible.
	The purpose of this file is for some future-proofing.
	Ideally we only define what we need, in a seperate namespace (NTNamespace).
	Not needed for now, but it's here for the future.
*/

#define NTNamespace

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

inline auto GetPlatformTickValue() -> std::intptr_t
{
	NTNamespace::LARGE_INTEGER i{};
	NTNamespace::QueryPerformanceCounter(&i);
	return static_cast<std::intptr_t>(i.QuadPart);
}

inline auto SecondsFromPlatformTickValue(std::intptr_t v) -> float
{
	NTNamespace::LARGE_INTEGER i{};
	NTNamespace::QueryPerformanceFrequency(&i);
	return static_cast<float>(static_cast<double>(v) / static_cast<double>(i.QuadPart));
}