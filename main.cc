#include <iostream>
#include <thread>
#include <string>

#include "W32Platform.h"
#include "DXRRenderer.h"
#include "W32Window.h"

int main(int, const char* const*, const char* const*)
{
	::HeapSetInformation(nullptr, ::HeapEnableTerminationOnCorruption, nullptr,
						 0);

	if (SUCCEEDED(::CoInitializeEx(nullptr, ::COINIT_MULTITHREADED)))
	{
		DXRRenderer renderer{};
		
		if (renderer.IsValid())
		{
			std::jthread updateWorker([&renderer](std::stop_token itoken) {
				auto lastTick = GetPlatformTickValue();
				while (!itoken.stop_requested())
				{
					const auto tick = GetPlatformTickValue();
					const auto time = SecondsFromPlatformTickValue(tick - lastTick);

					renderer.Update(time);

					lastTick = tick; 
				}
				});
			while (1)
			{
				if (!W32Window::DispatchMessagesThisThread())
					break;

				renderer.Render();
			}
			updateWorker.request_stop();
			updateWorker.join();
		}
		else
		{
			::MessageBoxA(0, "Failed to create renderer", "Error", MB_OK);
		}
		

		CoUninitialize();
	}

	return 0;
}
