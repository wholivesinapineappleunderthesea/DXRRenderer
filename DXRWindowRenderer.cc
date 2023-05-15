#include "DXRWindowRenderer.h"
#include <d3dcompiler.h>

#ifdef _DEBUG
#include <comdef.h>
#include <iostream>
#include <sstream>
#endif

#include <glm/gtc/matrix_transform.hpp>

DXRWindowRenderer::DXRWindowRenderer(W32Window* window)
{
	m_window = window;
	if (m_window)
	{
		if (m_window->IsValid())
		{
			NTNamespace::RECT rect{};
			if (NTNamespace::GetClientRect(m_window->NativeHandle(), &rect))
			{
				m_width = rect.right - rect.left;
				m_height = rect.bottom - rect.top;
			}
		}
	}
}

DXRWindowRenderer::~DXRWindowRenderer()
{
	if (m_d3dFence)
	{
		SignalFence();
		WaitFence();
	}
}

auto DXRWindowRenderer::DebugPrint(std::string_view msg) -> void
{
#ifdef _DEBUG
	std::cout << msg << std::endl;
#endif
}

auto DXRWindowRenderer::TestSucceeded(NTNamespace::HRESULT hr) -> bool
{
	if (SUCCEEDED(hr))
		return true;

	return false;
}

auto DXRWindowRenderer::CompileShaderToByteCode(const char* source,
												size_t& size_out,
												const char* entry,
												const char* target)
	-> unsigned char*
{
	COMPtr<::ID3DBlob> shaderBlob;
	COMPtr<::ID3DBlob> errorBlob;
	auto hr =
		::D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, entry,
					 target, 0, 0, shaderBlob.Out(), errorBlob.Out());
	if (!DXRSUCCESSTEST(hr))
	{
		if (errorBlob)
		{
			std::string err{};
			err.assign(reinterpret_cast<char*>(errorBlob->GetBufferPointer()),
					   errorBlob->GetBufferSize());
			DebugPrint(err);
		}
		DXRASSERT(0);
	}
	DXRASSERT(shaderBlob);
	size_out = shaderBlob->GetBufferSize();
	const auto buffer = new unsigned char[size_out];
	memcpy(buffer, shaderBlob->GetBufferPointer(), size_out);
	return buffer;
}

auto DXRWindowRenderer::DeviceLost() -> void
{
	for (NTNamespace::UINT n{}; n < k_NumSwapChainBuffers; n++)
	{
		m_d3dCommandAllocators[n].Reset();
		m_d3dRenderTargets[n].Reset();
#ifndef DXRDISABLED2D
		m_d3d11WrappedRenderTargets[n].Reset();
		m_d2dRenderTargets[n].Reset();
#endif
	}
#ifndef DXRDISABLED2D
	m_d2dSolidColorBrush.Reset();
	m_d2dDeviceContext.Reset();
	m_d2dDevice.Reset();
	m_d2dFactory.Reset();
	m_d3d11On12Device.Reset();
	m_d3d11DeviceContext.Reset();
	m_d3d11Device.Reset();
#endif

	m_d3dFence.Reset();
	m_d3dDepthStencilBuffer.Reset();
	m_d3dDsvDescriptorHeap.Reset();
	m_d3dPipelineState.Reset();
	m_d3dSrvDescriptorHeap.Reset();
	m_d3dRtvDescriptorHeap.Reset();
	m_d3dVertexBuffer.Reset();
	m_d3dRootSignature.Reset();
	m_d3dCommandList.Reset();
	m_d3dRtvDescriptorHeap.Reset();
	m_d3dCommandQueue.Reset();
	m_d3dDevice.Reset();
	m_dxgiSwapChain.Reset();
	m_dxgiAdapter.Reset();
	m_dxgiFactory.Reset();
#ifdef _DEBUG
	m_d3dDebug.Reset();
	m_dxgiDebug.Reset();
#endif

	ValidateAndCreateObjects();
}

auto DXRWindowRenderer::OnResize(NTNamespace::UINT w, NTNamespace::UINT h)
-> void
{
	std::lock_guard<std::mutex> lock{m_renderExecutionMutex};
	if (m_width != w || m_height != h)
	{
		m_width = w;
		m_height = h;

		if (m_dxgiSwapChain)
		{
			if (m_width > 0 && m_height > 0)
			{
				if (m_d3dCommandQueue && m_d3dFence)
				{
					SignalFence();
					WaitFence();
				}

#ifndef DXRDISABLED2D
				if (m_d2dDeviceContext)
					m_d2dDeviceContext->SetTarget(nullptr);
				if (m_d3d11DeviceContext)
					m_d3d11DeviceContext->Flush();

				m_d2dSolidColorBrush.Reset();
				for (auto& v : m_d2dRenderTargets)
				{
					v.Reset();
				}
				m_d2dDeviceContext.Reset();
				m_d2dDevice.Reset();
				m_d2dFactory.Reset();

				if (m_d3d11On12Device)
				{
					for (auto& v : m_d3d11WrappedRenderTargets)
					{
						if (v.Get())
						{
							ID3D11Resource* ppResources[] = {v.Get()};
							m_d3d11On12Device->ReleaseWrappedResources(
								ppResources, 1);
							v.Reset();
						}
					}
				}

				m_d3d11On12Device.Reset();
				m_d3d11DeviceContext.Reset();
				m_d3d11Device.Reset();

#endif

				m_fenceValue = 0;

				for (auto& v : m_d3dRenderTargets)
				{
					v.Reset();
				}

				m_d3dDepthStencilBuffer.Reset();

				::DXGI_SWAP_CHAIN_DESC1 desc{};
				DXRASSERT(DXRSUCCESSTEST(m_dxgiSwapChain->GetDesc1(&desc)));
				DXRASSERT(DXRSUCCESSTEST(m_dxgiSwapChain->ResizeBuffers(
					k_NumSwapChainBuffers, m_width, m_height, desc.Format,
					desc.Flags)));
			}
		}
	}
}

auto DXRWindowRenderer::ValidateAndCreateObjects() -> bool
{
	if (!CreateDXGIFactoryAndAdapter())
		return false;

	if (!CreateD3D12DeviceAndSwapChain())
		return false;

	if (!CreateD3D12Heaps())
		return false;

	if (!CreateD3D12RenderTargets())
		return false;

	if (!CreateD3D12CommandAllocators())
		return false;

	if (!CreateD3D12CommandList())
		return false;

	if (!CreateD3D12Fence())
		return false;

	if (!CreateD3D12RootSignature())
		return false;

	if (!CreateD3D12PipelineState())
		return false;

	if (!CreateD3D12DepthBuffer())
		return false;

#ifndef DXRDISABLED2D

	if (!CreateD3D11On12DeviceAndContext())
		return false;

	if (!CreateD2DFactory())
		return false;

	if (!CreateD2DDeviceAndContext())
		return false;

	if (!CreateD2DBrushes())
		return false;

	if (!CreateD2D1RenderTargets())
		return false;

#endif

	return true;
}

auto DXRWindowRenderer::RenderAll() -> bool
{
	std::lock_guard<std::mutex> lock{m_renderExecutionMutex};

	if (!ValidateAndCreateObjects())
		return false;

	if (!LoadRenderingAssets())
		return false;

	SubmitD3D12();

#ifndef DXRDISABLED2D

	SubmitD2D();

#endif

	SignalFence();

	auto hr = m_dxgiSwapChain->Present(m_presentWithVsync ? 1 : 0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		DeviceLost();
	}

	WaitFence();

	m_previousFrameIndex = m_frameIndex;
	m_frameIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	return true;
}

auto DXRWindowRenderer::Update(float dt) -> void
{
	m_cameraSinTime += dt;

	m_d3dViewport.Height = static_cast<float>(m_height);
	m_d3dViewport.Width = static_cast<float>(m_width);
	m_d3dViewport.MaxDepth = 1.0f;
	m_d3dViewport.MinDepth = 0.0f;
	m_d3dViewport.TopLeftX = 0.0f;
	m_d3dViewport.TopLeftY = 0.0f;

	m_d3dScissorRect.left = 0;
	m_d3dScissorRect.top = 0;
	m_d3dScissorRect.right = static_cast<NTNamespace::LONG>(m_width);
	m_d3dScissorRect.bottom = static_cast<NTNamespace::LONG>(m_height);

	m_cameraPosition.x = std::sinf(m_cameraSinTime) * 5.f;
	m_cameraPosition.y = 0.f;
	m_cameraPosition.z = std::cosf(m_cameraSinTime) * 5.f;

	const auto lookingAt = glm::vec3{};

	m_cameraForward = glm::normalize(lookingAt - m_cameraPosition);
	m_cameraRight = glm::cross(m_cameraForward, glm::vec3{0.f, 1.f, 0.f});
	m_cameraUp = glm::cross(m_cameraRight, m_cameraForward);

	m_cameraAspectRatio =
		static_cast<float>(m_width) / static_cast<float>(m_height);
	m_verticalFOV = m_horizontalFOV / m_cameraAspectRatio;

	m_projectionMatrix =
		glm::perspectiveRH_ZO(glm::radians(m_verticalFOV), m_cameraAspectRatio,
							  m_nearPlane, m_farPlane);
	m_projectionMatrix = glm::transpose(m_projectionMatrix);

	m_viewMatrix = glm::lookAtRH(
		m_cameraPosition, m_cameraPosition + m_cameraForward, m_cameraUp);
	m_viewMatrix = glm::transpose(m_viewMatrix);

	ProjAndViewMatrix p{};
	p.projectionMatrix = m_projectionMatrix;
	p.viewMatrix = m_viewMatrix;
	m_atomicCamera.store(p);
}