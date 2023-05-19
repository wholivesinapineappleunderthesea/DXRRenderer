#pragma once

// Define to disable D2D rendering alltogether:
#define DXRDISABLED2D

#include "COMPtr.h"
#include "W32Handle.h"
#include "W32Platform.h"

#include "W32Window.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#ifndef DXRDISABLED2D
#include <d2d1_1.h>
#include <d3d11on12.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <array>
#include <cassert>
#include <mutex>
#include <atomic>

#include <glm/glm.hpp>

#define DXRASSERT(x) assert(x)
#define DXRSUCCESSTEST DXRWindowRenderer::TestSucceeded

struct DXRWindowRenderer
{
	static auto DebugPrint(std::string_view msg) -> void;
	static auto TestSucceeded(NTNamespace::HRESULT hr) -> bool;

	DXRWindowRenderer(W32Window* window);
	~DXRWindowRenderer();

// Mark functions that should only be called from the win32 thread:
#define DXRWIN32THREAD

	// Called by the win32 wndproc handler (on a seperate thread):
	auto OnResize(NTNamespace::UINT w,
				  NTNamespace::UINT h) DXRWIN32THREAD->void;

	// Compiles a shader to bytecode:
	// Returns an owning pointer from new unsigned char[].
	static auto CompileShaderToByteCode(const char* source, size_t& size_out,
										const char* entry = "main",
										const char* target = "vs_5_0")
		-> unsigned char*;

	auto DeviceLost() -> void;

	// Responsible for m_dxgiFactory and m_dxgiAdapter:
	auto CreateDXGIFactoryAndAdapter() -> bool;

	// m_d3dDevice, m_d3dCommandQueue, m_d3dSwapChain:
	auto CreateD3D12DeviceAndSwapChain() -> bool;

	// m_d3dRtvDescriptorHeap, m_d3dDsvDescriptorHeap:
	auto CreateD3D12Heaps() -> bool;

	// m_d3dRenderTargets:
	// Render targets are swapchain size dependent:
	auto CreateD3D12RenderTargets() -> bool;

	// m_d3dCommandAllocators:
	auto CreateD3D12CommandAllocators() -> bool;

	// m_d3dCommandList:
	auto CreateD3D12CommandList() -> bool;

	// m_d3dFence:
	auto CreateD3D12Fence() -> bool;

	// m_d3dRootSignature:
	auto CreateD3D12RootSignature() -> bool;

	// m_d3dPipelineState:
	auto CreateD3D12PipelineState() -> bool;

	// m_d3dDepthStencilBuffer
	// Depth stencil buffer is swapchain size dependent:
	auto CreateD3D12DepthBuffer() -> bool;

	auto
	CreateD3D12TextureFromImageData(const void* imageData, int size,
									COMPtr<::ID3D12Resource>& textureResource)
		-> bool;

#ifndef DXRDISABLED2D
	// All D2D related resources are swapchain size dependent:

	// m_d3d11Device, m_d3d11DeviceContext, m_d3d11on12Device:
	auto CreateD3D11On12DeviceAndContext() -> bool;

	// m_d2dFactory:
	auto CreateD2DFactory() -> bool;

	// m_d2dDevice, m_d2dDeviceContext:
	auto CreateD2DDeviceAndContext() -> bool;

	// m_d3d11WrappedRenderTargets, m_d2dRenderTargets:
	auto CreateD2D1RenderTargets() -> bool;

	// m_d2dSolidColorBrush:
	auto CreateD2DBrushes() -> bool;
#endif

	// Creates Vertex3D buffer:
	auto CreateD3D12GPUUploadBuffer(std::intptr_t size,
									COMPtr<::ID3D12Resource>& resourceOut)
		-> bool;

	// Creates assets:
	auto LoadRenderingAssets() -> bool;

	// Creates all objects that are needed for rendering:
	auto ValidateAndCreateObjects() -> bool;

	// Signals m_d3dFence:
	auto SignalFence() -> void;
	// Wait for m_d3dFence:
	auto WaitFence() -> void;

	// Submits the D3D12 command list to the command queue:
	auto SubmitD3D12() -> void;
	// Submits D2D's command list to the command queue:
	auto SubmitD2D() -> void;

	// Render a frame, wait until finished:
	auto RenderAll() -> bool;

	// Updates everything:
	auto Update(float dt) -> void;

	// Viewport:
	::D3D12_VIEWPORT m_d3dViewport{};
	::D3D12_RECT m_d3dScissorRect{};

	// Vertex layout struct:
	struct Vertex3D
	{
		float x, y, z;
		float nx, ny, nz;
		float u, v;
		uint32_t col;
	};
	// Constant buffer for shaders:
	struct GraphicsConstants
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		static_assert(sizeof(float[4][4]) == sizeof(glm::mat4),
					  "Something is terribly wrong! The union is invalid...");
	};

	static constexpr auto k_GraphicsConstantsSize{sizeof(GraphicsConstants)};
	static constexpr auto k_GraphicsConstantsNum32Bit{k_GraphicsConstantsSize /
													  4};

  private:
	// Double-buffered:
	// Use m_frameIndex to index the current buffer.
	static inline constexpr auto k_NumSwapChainBuffers{2};

	// Clear color:
	static inline constexpr float k_ClearColor[]{0.0f, 0.0f, 0.0f, 0.0f};

	// Window related things:
	W32Window* m_window{};
	NTNamespace::UINT m_width{}, m_height{};
	bool m_presentWithVsync{};

	// RenderAll() will lock this mutex.
	// Window events come from a different thread, they will also lock this when
	// needed.
	// TODO: make COMPtr atomic?
	std::mutex m_renderExecutionMutex{};

#ifdef _DEBUG
	// Debug layer:
	COMPtr<::ID3D12Debug> m_d3dDebug{};
	COMPtr<::IDXGIDebug1> m_dxgiDebug{};
#endif

	// Used to mark which resources need to be released on resize:
#define DXRSWAPCHAINSIZEDEPENDENT

	// DXGI:
	using DXGIMinFactory_t = ::IDXGIFactory4;
	using DXGIMinAdapter_t = ::IDXGIAdapter1;
	using DXGIMinSwapChain_t = ::IDXGISwapChain3;
	// Factory:
	COMPtr<DXGIMinFactory_t> m_dxgiFactory{};
	// Adapter:
	COMPtr<DXGIMinAdapter_t> m_dxgiAdapter{};
	// Swap chain:
	COMPtr<DXGIMinSwapChain_t> m_dxgiSwapChain{};
	// Set if fallback to WARP was needed:
	bool m_isWARPAdapter{};

	// D3D12:
	// Device:
	::D3D_FEATURE_LEVEL m_d3dDeviceFeatureLevel{};
	COMPtr<::ID3D12Device> m_d3dDevice{};

	// Commands:
	std::array<COMPtr<::ID3D12CommandAllocator>, k_NumSwapChainBuffers>
		m_d3dCommandAllocators{};
	COMPtr<::ID3D12GraphicsCommandList> m_d3dCommandList{};
	COMPtr<::ID3D12CommandQueue> m_d3dCommandQueue{};

	// Render target objects:
	COMPtr<::ID3D12DescriptorHeap> m_d3dRtvDescriptorHeap{};
	NTNamespace::UINT m_d3dRtvDescriptorSize{};
	std::array<COMPtr<::ID3D12Resource>, k_NumSwapChainBuffers>
		DXRSWAPCHAINSIZEDEPENDENT m_d3dRenderTargets{};

	// Root signature:
	COMPtr<::ID3D12RootSignature> m_d3dRootSignature{};

	// Mesh objects:
	COMPtr<::ID3D12Resource> m_d3dVertexBuffer{};
	::D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView{};

	// Texture objects:
	COMPtr<::ID3D12Resource> m_d3dTexture{};
	COMPtr<::ID3D12DescriptorHeap> m_d3dSrvDescriptorHeap{};

	// Pipeline states:
	COMPtr<::ID3D12PipelineState> m_d3dPipelineState{};

	// Depth stencil objects:
	COMPtr<::ID3D12DescriptorHeap> m_d3dDsvDescriptorHeap{};
	NTNamespace::UINT m_d3dDsvDescriptorSize{};
	COMPtr<::ID3D12Resource> DXRSWAPCHAINSIZEDEPENDENT
		m_d3dDepthStencilBuffer{};

	// Fence:
	COMPtr<::ID3D12Fence> m_d3dFence{};
	W32Handle<nullptr> m_fenceEvent{};
	NTNamespace::UINT64 m_fenceValue{};

	// The backbuffer/rendertarget index:
	NTNamespace::UINT m_frameIndex{};
	NTNamespace::UINT m_previousFrameIndex{};

	// Camera constants:
	struct CameraMatrices
	{
		glm::mat4 projection;
		glm::mat4 view;
	};
	std::atomic<CameraMatrices> m_atomicCamera{};;

#ifndef DXRDISABLED2D

// ALL D2D/D3D11On12 OBJECTS ARE SWAPCHAIN SIZE DEPENDENT!

// D3D11On12:
// Device:
COMPtr<::ID3D11Device> DXRSWAPCHAINSIZEDEPENDENT m_d3d11Device{};
COMPtr<::ID3D11DeviceContext> DXRSWAPCHAINSIZEDEPENDENT m_d3d11DeviceContext{};
COMPtr<::ID3D11On12Device> DXRSWAPCHAINSIZEDEPENDENT m_d3d11On12Device{};

// D3D11 render target objects:
std::array<COMPtr<::ID3D11Resource>, k_NumSwapChainBuffers>
	DXRSWAPCHAINSIZEDEPENDENT m_d3d11WrappedRenderTargets{};

// D2D1:
// Factory:
COMPtr<::ID2D1Factory1> DXRSWAPCHAINSIZEDEPENDENT m_d2dFactory{};
// Device:
COMPtr<::ID2D1Device> DXRSWAPCHAINSIZEDEPENDENT m_d2dDevice{};
COMPtr<::ID2D1DeviceContext> DXRSWAPCHAINSIZEDEPENDENT m_d2dDeviceContext{};
// Render target objects:
std::array<COMPtr<::ID2D1Bitmap1>, k_NumSwapChainBuffers>
	DXRSWAPCHAINSIZEDEPENDENT m_d2dRenderTargets{};
// Brushes:
COMPtr<::ID2D1SolidColorBrush> DXRSWAPCHAINSIZEDEPENDENT m_d2dSolidColorBrush{};
#endif
}
;