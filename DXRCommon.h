#pragma once

#include <cstdint>
#include <utility>

#define THREAD_MARKER(x)

struct DXRNonCopyable
{
	DXRNonCopyable() = default;
	DXRNonCopyable(const DXRNonCopyable&) = delete;
	DXRNonCopyable(DXRNonCopyable&&) = delete;
	DXRNonCopyable& operator=(const DXRNonCopyable&) = delete;
	DXRNonCopyable& operator=(DXRNonCopyable&&) = delete;
};

#include "DXRSingleton.h"

#include <glm/glm.hpp>

using DXRVec2 = glm::vec2;
using DXRVec3 = glm::vec3;
using DXRVec4 = glm::vec4;
using DXRMat4 = glm::mat4;
using DXRQuat = glm::quat;