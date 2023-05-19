cmake_minimum_required (VERSION 3.20)

if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("DXRProj")
add_executable (DXRProj "main.cc" "W32Window.cc" "DXRWindowRendererD2D.cc" "DXRWindowRenderer.cc" "DXRWindowRendererD3D12.cc" "CameraManager.cc" "DXRSingletonInstances.cc")

set_property(TARGET DXRProj PROPERTY CXX_STANDARD 23)
set_property(TARGET DXRProj PROPERTY INTERPROCEDURAL_OPTIMIZATION)

if(MSVC)
	target_compile_options(DXRProj PRIVATE /W4 /WX /permissive- /w14640 /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928)
else()
	target_compile_options(DXRProj PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror -Wconversion -Wsign-conversion -Wnon-virtual-dtor -Wunused -Woverloaded-virtual)
endif()

# vendor headers
target_include_directories(DXRProj PRIVATE "vendor")

# dx libs
target_link_libraries(DXRProj PRIVATE "d2d1")
target_link_libraries(DXRProj PRIVATE "d3d11")
target_link_libraries(DXRProj PRIVATE "d3d12")
target_link_libraries(DXRProj PRIVATE "dxgi")
target_link_libraries(DXRProj PRIVATE "d3dcompiler")