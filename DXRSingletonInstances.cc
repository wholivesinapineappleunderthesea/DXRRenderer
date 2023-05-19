#include "DXRSingleton.h"

#include "CameraManager.h"

struct AllEngineSingletons
{
	DXRSingleton<CameraManager> g_cameraManager{};
};

auto InitializeSingletonInstances() -> void
{
	static AllEngineSingletons g_Singletons{};
}