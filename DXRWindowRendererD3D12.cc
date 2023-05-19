#include "DXRWindowRenderer.h"
#include "ShaderSources.h"

#pragma warning(push)
#pragma warning(disable : 4324)
#include "d3dx12.h"
#pragma warning(pop)

#include "RawImage.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

auto DXRWindowRenderer::CreateDXGIFactoryAndAdapter() -> bool
{
	if (!m_dxgiFactory)
	{

		NTNamespace::UINT dxgiFactoryFlags{};

		// Debugging DXGI:
#ifdef _DEBUG
		if (!m_d3dDebug)
		{
			if (DXRSUCCESSTEST(::D3D12GetDebugInterface(m_d3dDebug.static_uuid,
														m_d3dDebug.Out())))
			{
				DXRASSERT(m_d3dDebug);
				m_d3dDebug->EnableDebugLayer();
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}

		if (!m_dxgiDebug)
		{
			if (SUCCEEDED(::DXGIGetDebugInterface1(0, m_dxgiDebug.static_uuid,
												   m_dxgiDebug.Out())))
			{
				DXRASSERT(m_dxgiDebug);
				m_dxgiDebug->EnableLeakTrackingForThread();
			}
		}

#endif

		if (DXRSUCCESSTEST(::CreateDXGIFactory2(dxgiFactoryFlags,
												m_dxgiFactory.static_uuid,
												m_dxgiFactory.Out())))
		{
			DXRASSERT(m_dxgiFactory);
			// Disable Alt+Enter fullscreen toggle feature:
			m_dxgiFactory->MakeWindowAssociation(m_window->NativeHandle(),
												 DXGI_MWA_NO_ALT_ENTER);
		}
	}

	if (!m_dxgiFactory)
		return false;

	if (!m_dxgiAdapter)
	{
		m_d3dDeviceFeatureLevel = {};
#define TESTADAPTERFEATUREMACRO(adapter, level)                                \
	if (DXRSUCCESSTEST(::D3D12CreateDevice(adapter.Get(), level,               \
										   __uuidof(::ID3D12Device), 0)))      \
	{                                                                          \
		m_d3dDeviceFeatureLevel = level;                                       \
		canUse = true;                                                         \
	}

		COMPtr<DXGIMinAdapter_t> adapter{};
		for (NTNamespace::UINT idx = 0;
			 DXRSUCCESSTEST(m_dxgiFactory->EnumAdapters1(idx, adapter.Out()));
			 ++idx)
		{
			if (!adapter)
				continue;
			::DXGI_ADAPTER_DESC1 adapterDesc{};
			if (!DXRSUCCESSTEST(adapter->GetDesc1(&adapterDesc)))
				continue;
			if (adapterDesc.Flags & ::DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			bool canUse = false;

			TESTADAPTERFEATUREMACRO(adapter, ::D3D_FEATURE_LEVEL_12_0);
			TESTADAPTERFEATUREMACRO(adapter, ::D3D_FEATURE_LEVEL_12_1);
			TESTADAPTERFEATUREMACRO(adapter, ::D3D_FEATURE_LEVEL_12_2);
			if (canUse)
			{
				m_dxgiAdapter = std::move(adapter);
				DXRASSERT(m_dxgiAdapter);
				return m_dxgiAdapter;
			}
		}

		if (!m_dxgiAdapter)
		{
			// We probably don't have a D3D12 capable device.
			// But we can use WARP.
			if (DXRSUCCESSTEST(m_dxgiFactory->EnumWarpAdapter(
					__uuidof(DXGIMinAdapter_t), adapter.Out())))
			{
				m_isWARPAdapter = true;

				bool canUse = false;

				TESTADAPTERFEATUREMACRO(adapter, ::D3D_FEATURE_LEVEL_12_0);
				TESTADAPTERFEATUREMACRO(adapter, ::D3D_FEATURE_LEVEL_12_1);
				TESTADAPTERFEATUREMACRO(adapter, ::D3D_FEATURE_LEVEL_12_2);
				if (m_d3dDeviceFeatureLevel)
				{
					m_dxgiAdapter = std::move(adapter);
					DXRASSERT(m_dxgiAdapter);
				}
			}
		}
	}
	return m_dxgiAdapter;

#undef TESTADAPTERFEATUREMACRO
}

auto DXRWindowRenderer::CreateD3D12DeviceAndSwapChain() -> bool
{
	DXRASSERT(m_dxgiFactory);
	DXRASSERT(m_dxgiAdapter);
	if (!m_d3dDevice)
	{
		if (DXRSUCCESSTEST(::D3D12CreateDevice(
				m_dxgiAdapter.Get(), m_d3dDeviceFeatureLevel,
				__uuidof(::ID3D12Device), m_d3dDevice.Out())))
		{
			DXRASSERT(m_d3dDevice);
			m_d3dDevice->SetName(L"m_d3dDevice");
		}
	}

	if (!m_d3dDevice)
		return false;

	if (!m_d3dCommandQueue)
	{
		::D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Type = ::D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = ::D3D12_COMMAND_QUEUE_FLAG_NONE;
		if (DXRSUCCESSTEST(m_d3dDevice->CreateCommandQueue(
				&queueDesc, m_d3dCommandQueue.static_uuid,
				m_d3dCommandQueue.Out())))
		{
			DXRASSERT(m_d3dCommandQueue);
			m_d3dCommandQueue->SetName(L"m_d3dCommandQueue");
		}
	}

	if (!m_d3dCommandQueue)
		return false;

	if (!m_dxgiSwapChain)
	{
		::DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

		swapChainDesc.Width = m_width;
		swapChainDesc.Height = m_height;
		swapChainDesc.Format = ::DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = k_NumSwapChainBuffers;
		swapChainDesc.Scaling = ::DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = ::DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = ::DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = 0;
		COMPtr<::IDXGISwapChain1> swapChain{};
		if (DXRSUCCESSTEST(m_dxgiFactory->CreateSwapChainForHwnd(
				m_d3dCommandQueue.Get(), m_window->NativeHandle(),
				&swapChainDesc, nullptr, nullptr, swapChain.Out())))
		{
			if (DXRSUCCESSTEST(swapChain->QueryInterface(
					m_dxgiSwapChain.static_uuid, m_dxgiSwapChain.Out())))
			{
				DXRASSERT(m_dxgiSwapChain);
			}
		}
	}

	return m_dxgiSwapChain;
}

auto DXRWindowRenderer::CreateD3D12Heaps() -> bool
{
	DXRASSERT(m_d3dDevice);
	if (!m_d3dRtvDescriptorHeap)
	{
		::D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = k_NumSwapChainBuffers;
		rtvHeapDesc.Type = ::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = ::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (DXRSUCCESSTEST(m_d3dDevice->CreateDescriptorHeap(
				&rtvHeapDesc, m_d3dRtvDescriptorHeap.static_uuid,
				m_d3dRtvDescriptorHeap.Out())))
		{
			DXRASSERT(m_d3dRtvDescriptorHeap);

			m_d3dRtvDescriptorHeap->SetName(L"m_d3dRtvDescriptorHeap");

			m_d3dRtvDescriptorSize =
				m_d3dDevice->GetDescriptorHandleIncrementSize(
					::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}
	}
	if (!m_d3dRtvDescriptorHeap)
		return false;

	if (!m_d3dDsvDescriptorHeap)
	{
		::D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = ::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = ::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		auto hr = m_d3dDevice->CreateDescriptorHeap(
			&dsvHeapDesc, m_d3dDsvDescriptorHeap.static_uuid,
			m_d3dDsvDescriptorHeap.Out());
		DXRASSERT(DXRSUCCESSTEST(hr));
		m_d3dDsvDescriptorHeap->SetName(L"m_d3dDsvDescriptorHeap");
		m_d3dDsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(
			::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}
	if (!m_d3dDsvDescriptorHeap)
		return false;

	if (!m_d3dSrvDescriptorHeap)
	{
		::D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = ::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = ::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (DXRSUCCESSTEST(m_d3dDevice->CreateDescriptorHeap(
				&srvHeapDesc, m_d3dSrvDescriptorHeap.static_uuid,
				m_d3dSrvDescriptorHeap.Out())))
		{
			DXRASSERT(m_d3dSrvDescriptorHeap);
			m_d3dSrvDescriptorHeap->SetName(L"m_d3dSrvDescriptorHeap");
		}
	}
	if (!m_d3dSrvDescriptorHeap)
		return false;
	return true;
}

auto DXRWindowRenderer::CreateD3D12RenderTargets() -> bool
{
	DXRASSERT(m_d3dRtvDescriptorHeap);
	DXRASSERT(m_dxgiSwapChain);
	DXRASSERT(m_d3dDevice);
	auto rtvHandle{
		m_d3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};

	for (NTNamespace::UINT n{}; n < m_d3dRenderTargets.size(); n++)
	{
		if (!m_d3dRenderTargets[n])
		{
			DXRASSERT(DXRSUCCESSTEST(m_dxgiSwapChain->GetBuffer(n, __uuidof(::ID3D12Resource),
									   m_d3dRenderTargets[n].Out())));
			DXRASSERT(m_d3dRenderTargets[n]);
			m_d3dRenderTargets[n]->SetName(L"m_d3dRenderTargets[n]");
			m_d3dDevice->CreateRenderTargetView(m_d3dRenderTargets[n].Get(),
												nullptr, rtvHandle);
			rtvHandle.ptr += m_d3dRtvDescriptorSize;

			m_frameIndex = 0;
			m_previousFrameIndex = 1;
		}
	}
	return true;
}

auto DXRWindowRenderer::CreateD3D12CommandAllocators() -> bool
{
	DXRASSERT(m_d3dDevice);
	for (auto& v : m_d3dCommandAllocators)
	{
		if (!v)
		{
			if (DXRSUCCESSTEST(m_d3dDevice->CreateCommandAllocator(
					::D3D12_COMMAND_LIST_TYPE_DIRECT, v.static_uuid, v.Out())))
			{
				DXRASSERT(v);
				v->SetName(L"m_d3dCommandAllocator");
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

auto DXRWindowRenderer::CreateD3D12CommandList() -> bool
{
	DXRASSERT(m_d3dDevice);
	if (!m_d3dCommandList)
	{
		if (DXRSUCCESSTEST(m_d3dDevice->CreateCommandList(
				0, ::D3D12_COMMAND_LIST_TYPE_DIRECT,
				m_d3dCommandAllocators[0].Get(), nullptr,
				m_d3dCommandList.static_uuid, m_d3dCommandList.Out())))
		{
			DXRASSERT(m_d3dCommandList);
			m_d3dCommandList->SetName(L"m_d3dCommandList");

			DXRASSERT(DXRSUCCESSTEST(m_d3dCommandList->Close()));
		}
	}
	return m_d3dCommandList;
}

auto DXRWindowRenderer::CreateD3D12Fence() -> bool
{
	DXRASSERT(m_d3dDevice);
	if (!m_d3dFence)
	{
		if (DXRSUCCESSTEST(m_d3dDevice->CreateFence(0, ::D3D12_FENCE_FLAG_NONE,
													m_d3dFence.static_uuid,
													m_d3dFence.Out())))
		{
			DXRASSERT(m_d3dFence);
			m_d3dFence->SetName(L"m_d3dFence");
			m_fenceEvent = ::CreateEventW(0, false, false, 0);
			DXRASSERT(m_fenceEvent);

			m_fenceValue = 1;
		}
	}
	return m_d3dFence;
}

auto DXRWindowRenderer::CreateD3D12RootSignature() -> bool
{
	DXRASSERT(m_d3dDevice);
	if (!m_d3dRootSignature)
	{
		::D3D12_DESCRIPTOR_RANGE descRange = {};
		descRange.RangeType = ::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descRange.NumDescriptors = 1;
		descRange.BaseShaderRegister = 0;
		descRange.RegisterSpace = 0;
		descRange.OffsetInDescriptorsFromTableStart = 0;

		::D3D12_ROOT_PARAMETER param[2] = {};

		param[0].ParameterType = ::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		param[0].Constants.ShaderRegister = 0;
		param[0].Constants.RegisterSpace = 0;
		param[0].Constants.Num32BitValues = k_GraphicsConstantsNum32Bit;
		param[0].ShaderVisibility = ::D3D12_SHADER_VISIBILITY_VERTEX;

		param[1].ParameterType = ::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[1].DescriptorTable.NumDescriptorRanges = 1;
		param[1].DescriptorTable.pDescriptorRanges = &descRange;
		param[1].ShaderVisibility = ::D3D12_SHADER_VISIBILITY_PIXEL;

		::D3D12_STATIC_SAMPLER_DESC staticSampler{};
		staticSampler.Filter = ::D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticSampler.AddressU = ::D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampler.AddressV = ::D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampler.AddressW = ::D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSampler.MipLODBias = 0.f;
		staticSampler.MaxAnisotropy = 0;
		staticSampler.ComparisonFunc = ::D3D12_COMPARISON_FUNC_NEVER;
		staticSampler.BorderColor =
			::D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampler.MinLOD = 0.f;
		staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticSampler.ShaderRegister = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = ::D3D12_SHADER_VISIBILITY_PIXEL;

		::D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = _countof(param);
		desc.pParameters = param;
		desc.NumStaticSamplers = 1;
		desc.pStaticSamplers = &staticSampler;
		desc.Flags =
			::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		COMPtr<::ID3DBlob> signature;
		COMPtr<::ID3DBlob> error;
		auto hr =
			::D3D12SerializeRootSignature(&desc, ::D3D_ROOT_SIGNATURE_VERSION_1,
										  signature.Out(), error.Out());
		DXRASSERT(DXRSUCCESSTEST(hr));
		DXRASSERT(signature);
		hr = m_d3dDevice->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			m_d3dRootSignature.static_uuid, m_d3dRootSignature.Out());
		DXRASSERT(DXRSUCCESSTEST(hr));
		DXRASSERT(m_d3dRootSignature);
		m_d3dRootSignature->SetName(L"m_d3dRootSignature");
		return true;
	}
	return m_d3dRootSignature;
}

auto DXRWindowRenderer::CreateD3D12PipelineState() -> bool
{
	DXRASSERT(m_d3dDevice);
	if (!m_d3dPipelineState)
	{
		size_t vertexShaderbcSize{};
		const auto vertexShaderbc = CompileShaderToByteCode(
			vertexShader2DText, vertexShaderbcSize, "main", "vs_5_0");
		DXRASSERT(vertexShaderbc);
		size_t pixelShaderbcSize{};
		const auto pixelShaderbc = CompileShaderToByteCode(
			pixelShader2DText, pixelShaderbcSize, "main", "ps_5_0");
		DXRASSERT(pixelShaderbc);
		::D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = m_d3dRootSignature.Get();

		psoDesc.VS.pShaderBytecode = vertexShaderbc;
		psoDesc.VS.BytecodeLength = vertexShaderbcSize;

		psoDesc.PS.pShaderBytecode = pixelShaderbc;
		psoDesc.PS.BytecodeLength = pixelShaderbcSize;

		psoDesc.BlendState.AlphaToCoverageEnable = false;
		psoDesc.BlendState.IndependentBlendEnable = false;
		psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = ::D3D12_BLEND_SRC_ALPHA;
		psoDesc.BlendState.RenderTarget[0].DestBlend =
			::D3D12_BLEND_INV_SRC_ALPHA;
		psoDesc.BlendState.RenderTarget[0].BlendOp = ::D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = ::D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = ::D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = ::D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
			::D3D12_COLOR_WRITE_ENABLE_ALL;

		psoDesc.SampleMask = UINT_MAX;

		psoDesc.RasterizerState.FillMode = ::D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = ::D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.FrontCounterClockwise = false;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias =
			D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.RasterizerState.MultisampleEnable = false;
		psoDesc.RasterizerState.AntialiasedLineEnable = false;
		psoDesc.RasterizerState.ForcedSampleCount = 0;
		psoDesc.RasterizerState.ConservativeRaster =
			::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthWriteMask = ::D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = ::D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.StencilEnable = false;
		psoDesc.DepthStencilState.FrontFace.StencilPassOp =
			::D3D12_STENCIL_OP_KEEP;
		psoDesc.DepthStencilState.FrontFace.StencilFailOp =
			::D3D12_STENCIL_OP_KEEP;
		psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp =
			::D3D12_STENCIL_OP_KEEP;
		psoDesc.DepthStencilState.BackFace =
			psoDesc.DepthStencilState.FrontFace;

		::D3D12_INPUT_ELEMENT_DESC layout[] = {
			{"POSITION", 0, ::DXGI_FORMAT_R32G32B32_FLOAT, 0,
			 offsetof(Vertex3D, x),
			 ::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, ::DXGI_FORMAT_R32G32B32_FLOAT, 0,
			 offsetof(Vertex3D, nx),
			 ::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, ::DXGI_FORMAT_R32G32_FLOAT, 0,
			 offsetof(Vertex3D, u),
			 ::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, ::DXGI_FORMAT_R8G8B8A8_UNORM, 0,
			 offsetof(Vertex3D, col),
			 ::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		psoDesc.InputLayout = {layout, 4};

		psoDesc.PrimitiveTopologyType =
			::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = ::DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = ::DXGI_FORMAT_D32_FLOAT;

		psoDesc.SampleDesc.Count = 1;

		psoDesc.NodeMask = 1;

		psoDesc.Flags = ::D3D12_PIPELINE_STATE_FLAG_NONE;

		auto hr = m_d3dDevice->CreateGraphicsPipelineState(
			&psoDesc, m_d3dPipelineState.static_uuid, m_d3dPipelineState.Out());
		DXRASSERT(DXRSUCCESSTEST(hr));
		m_d3dPipelineState->SetName(L"m_d3dPipelineState");
		delete[] vertexShaderbc;
		delete[] pixelShaderbc;
	}
	return m_d3dPipelineState;
}

auto DXRWindowRenderer::CreateD3D12DepthBuffer() -> bool
{
	DXRASSERT(m_d3dDevice);
	if (!m_d3dDepthStencilBuffer)
	{
		::D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = ::D3D12_HEAP_TYPE_DEFAULT;
		heapProperties.CPUPageProperty = ::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = ::D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;

		::D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = ::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = m_width;
		resourceDesc.Height = m_height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = ::DXGI_FORMAT_D32_FLOAT;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = ::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = ::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		::D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = ::DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		auto hr = m_d3dDevice->CreateCommittedResource(
			&heapProperties, ::D3D12_HEAP_FLAG_NONE, &resourceDesc,
			::D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
			m_d3dDepthStencilBuffer.static_uuid, m_d3dDepthStencilBuffer.Out());
		DXRASSERT(DXRSUCCESSTEST(hr));
		m_d3dDepthStencilBuffer->SetName(L"m_d3dDepthStencilBuffer");

		::D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = ::DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = ::D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = ::D3D12_DSV_FLAG_NONE;
		m_d3dDevice->CreateDepthStencilView(
			m_d3dDepthStencilBuffer.Get(), &dsvDesc,
			m_d3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}
	return m_d3dDepthStencilBuffer;
}
auto DXRWindowRenderer::CreateD3D12TextureFromImageData(const void* imageData, int size, COMPtr<::ID3D12Resource>& textureResource) -> bool
{
	DXRASSERT(m_d3dDevice);
	DXRASSERT(m_d3dCommandList);
	DXRASSERT(m_d3dCommandQueue);
	int width{};
	int height{};
	stbi_set_flip_vertically_on_load(1);
	const auto bitmap_image_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(imageData),size, &width, &height, NULL, 4);
	if (!bitmap_image_data)
		return false;

	::D3D12_HEAP_PROPERTIES defHeapProperties{};
	defHeapProperties.Type = ::D3D12_HEAP_TYPE_DEFAULT;
	defHeapProperties.CPUPageProperty = ::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defHeapProperties.MemoryPoolPreference = ::D3D12_MEMORY_POOL_UNKNOWN;
	defHeapProperties.CreationNodeMask = 1;
	defHeapProperties.VisibleNodeMask = 1;

	::D3D12_RESOURCE_DESC textureDesc{};
	textureDesc.Dimension = ::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = ::DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = ::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = ::D3D12_RESOURCE_FLAG_NONE;
	
	auto hr = m_d3dDevice->CreateCommittedResource(&defHeapProperties, ::D3D12_HEAP_FLAG_NONE, &textureDesc,
		::D3D12_RESOURCE_STATE_COPY_DEST, NULL, textureResource.static_uuid, textureResource.InOut());
	DXRASSERT(DXRSUCCESSTEST(hr));

	::D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = ::D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	const NTNamespace::UINT uploadpitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	const NTNamespace::UINT uploadsize = height * uploadpitch;
	bufferDesc.Width = uploadsize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = ::DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = ::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = ::D3D12_RESOURCE_FLAG_NONE;
	
	::D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = ::D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty =
		::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = ::D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 1;
	uploadHeapProperties.VisibleNodeMask = 1;

	COMPtr<::ID3D12Resource> uploadHeap{};
	hr = m_d3dDevice->CreateCommittedResource(&uploadHeapProperties, ::D3D12_HEAP_FLAG_NONE, &bufferDesc,
		::D3D12_RESOURCE_STATE_GENERIC_READ, 0, uploadHeap.static_uuid, uploadHeap.Out());
	DXRASSERT(DXRSUCCESSTEST(hr));

	void* mapped{};
	::D3D12_RANGE range{ 0, uploadsize };
	hr = uploadHeap->Map(0, &range, &mapped);
	DXRASSERT(DXRSUCCESSTEST(hr));
	for (auto y{0}; y < height; y++)
	{
		const auto dst = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(mapped) + y * uploadpitch);
		const auto src = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(bitmap_image_data) + y * width * 4);
		memcpy(dst, src, width * 4);
	}
	uploadHeap->Unmap(0, &range);

	

	

	DXRASSERT(DXRSUCCESSTEST(m_d3dCommandList->Reset(m_d3dCommandAllocators[m_frameIndex].Get(), m_d3dPipelineState.Get())));

	::D3D12_TEXTURE_COPY_LOCATION srcLocation{};
	srcLocation.pResource = uploadHeap.Get();
	srcLocation.Type = ::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLocation.PlacedFootprint.Footprint.Format = ::DXGI_FORMAT_R8G8B8A8_UNORM;
	srcLocation.PlacedFootprint.Footprint.Width = width;
	srcLocation.PlacedFootprint.Footprint.Height = height;
	srcLocation.PlacedFootprint.Footprint.Depth = 1;
	srcLocation.PlacedFootprint.Footprint.RowPitch = uploadpitch;

	::D3D12_TEXTURE_COPY_LOCATION dstLocation{};
	dstLocation.pResource = textureResource.Get();
	dstLocation.Type = ::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLocation.SubresourceIndex = 0;
	m_d3dCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);

	::D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = ::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = ::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = textureResource.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = ::D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = ::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_d3dCommandList->ResourceBarrier(1, &barrier);

	m_d3dCommandList->Close();

	::ID3D12CommandList* cmdList[] = { m_d3dCommandList.Get() };
	m_d3dCommandQueue->ExecuteCommandLists(1, cmdList);

	SignalFence();
	WaitFence();
	
	::D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = ::DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = ::D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_d3dDevice->CreateShaderResourceView(textureResource.Get(), &srvDesc, m_d3dSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	stbi_image_free(bitmap_image_data);

	return true;
}

auto DXRWindowRenderer::CreateD3D12GPUUploadBuffer(std::intptr_t size, COMPtr<::ID3D12Resource>& resourceOut)
	-> bool
{
	DXRASSERT(m_d3dDevice);

	::D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = ::D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = ::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = ::D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	::D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = ::D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = ::DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = ::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = ::D3D12_RESOURCE_FLAG_NONE;

	auto hr = m_d3dDevice->CreateCommittedResource(
		&heapProperties, ::D3D12_HEAP_FLAG_NONE, &resourceDesc,
		::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, resourceOut.static_uuid,
		resourceOut.InOut());
	DXRASSERT(DXRSUCCESSTEST(hr));
	return resourceOut;
}

auto DXRWindowRenderer::LoadRenderingAssets() -> bool
{
	if (!m_d3dVertexBuffer)
	{
		// x, y, z, nx, ny, nz, u, v, col
		Vertex3D vertices[] = {
			// -z
			{ -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF }, 
			{ -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			{  1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF },
			{ -1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0xFFFFFFFF }, 
			// +z
			{ -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF }, 
			{ -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			{  1.0f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF },
			{ -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0xFFFFFFFF }, 
			// -y
			{ -1.0f, -1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f, -1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF }, 
			{ -1.0f, -1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			{  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF },
			{ -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0xFFFFFFFF }, 
			// +y
			{ -1.0f, 1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f, 1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0xFFFFFFFF }, 
			{  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF }, 
			{ -1.0f, 1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			{  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF },
			{ -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0xFFFFFFFF }, 
			// -x
			{ -1.0f,-1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, 
			{ -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0xFFFFFFFF }, 
			{ -1.0f, 1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF }, 
			{ -1.0f,-1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			{ -1.0f, 1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF },
			{ -1.0f,-1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0xFFFFFFFF }, 
			// +x
			{ 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, 
			{ 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0xFFFFFFFF }, 
			{ 1.0f,  1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF }, 
			{ 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			{ 1.0f,  1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0xFFFFFFFF },
			{ 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0xFFFFFFFF }, 
		};

		DXRASSERT(CreateD3D12GPUUploadBuffer(sizeof vertices, m_d3dVertexBuffer));
		DXRASSERT(m_d3dVertexBuffer);
		m_d3dVertexBuffer->SetName(L"m_d3dVertexBuffer");
		void* pVertexDataBegin{};
		auto hr = m_d3dVertexBuffer->Map(0, nullptr, &pVertexDataBegin);
		DXRASSERT(DXRSUCCESSTEST(hr));
		memcpy(pVertexDataBegin, vertices, sizeof vertices);
		m_d3dVertexBuffer->Unmap(0, nullptr);

		m_d3dVertexBufferView.BufferLocation =
			m_d3dVertexBuffer->GetGPUVirtualAddress();
		m_d3dVertexBufferView.StrideInBytes = sizeof(Vertex3D);
		m_d3dVertexBufferView.SizeInBytes = sizeof vertices;
	}
	if (!m_d3dVertexBuffer)
		return false;

	if (!m_d3dTexture)
	{
		DXRASSERT(CreateD3D12TextureFromImageData(textureDataRaw, sizeof textureDataRaw, m_d3dTexture));
		DXRASSERT(m_d3dTexture);
		m_d3dTexture->SetName(L"m_d3dTexture");
	}

	if (!m_d3dTexture)
		return false;

	return true;
}

auto DXRWindowRenderer::SignalFence() -> void
{
	DXRASSERT(m_d3dCommandQueue);
	DXRASSERT(m_d3dFence);
	DXRASSERT(DXRSUCCESSTEST(
		m_d3dCommandQueue->Signal(m_d3dFence.Get(), m_fenceValue)));
}

auto DXRWindowRenderer::WaitFence() -> void
{
	DXRASSERT(m_d3dFence);
	DXRASSERT(m_fenceEvent);
	if (m_d3dFence->GetCompletedValue() < m_fenceValue)
	{
		DXRASSERT(DXRSUCCESSTEST(
			m_d3dFence->SetEventOnCompletion(m_fenceValue, m_fenceEvent)));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_fenceValue++;
}

auto DXRWindowRenderer::SubmitD3D12() -> void
{
	const auto cmdallocator = m_d3dCommandAllocators[m_frameIndex].Get();
	const auto rendertarget = m_d3dRenderTargets[m_frameIndex].Get();

	DXRASSERT(cmdallocator);
	DXRASSERT(rendertarget);
	DXRASSERT(m_d3dCommandQueue);
	DXRASSERT(m_d3dCommandList);
	DXRASSERT(m_d3dPipelineState);
	DXRASSERT(m_d3dRootSignature);
	DXRASSERT(m_d3dSrvDescriptorHeap);
	DXRASSERT(m_d3dRtvDescriptorHeap);
	DXRASSERT(m_d3dDsvDescriptorHeap);


	DXRASSERT(DXRSUCCESSTEST(cmdallocator->Reset()));

	DXRASSERT(DXRSUCCESSTEST(
		m_d3dCommandList->Reset(cmdallocator, m_d3dPipelineState.Get())));

	m_d3dCommandList->SetGraphicsRootSignature(m_d3dRootSignature.Get());

	::ID3D12DescriptorHeap* ppHeaps[] = {m_d3dSrvDescriptorHeap.Get()};
	m_d3dCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_d3dCommandList->SetGraphicsRootDescriptorTable(
		1, m_d3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	m_d3dCommandList->RSSetViewports(1, &m_d3dViewport);
	m_d3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);

	// Begin render target:
	::D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = ::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = ::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = rendertarget;
	barrier.Transition.Subresource = 0;
	barrier.Transition.StateBefore = ::D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = ::D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_d3dCommandList->ResourceBarrier(1, &barrier);

	// Clear render target:
	auto rtvHandle =
		m_d3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto dsvHandle =
		m_d3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_frameIndex * m_d3dRtvDescriptorSize;
	m_d3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	m_d3dCommandList->ClearDepthStencilView(dsvHandle, ::D3D12_CLEAR_FLAG_DEPTH,
											1.f, 0, 0, nullptr);
	m_d3dCommandList->ClearRenderTargetView(rtvHandle, k_ClearColor, 0,
											nullptr);

	// Set camera constants:
	GraphicsConstants constants{};
	const auto matrices = m_atomicCamera.load();
	memcpy(&constants.projection, &matrices.projection[0][0],
		   sizeof constants.projection);
	memcpy(&constants.view, &matrices.view[0][0], sizeof constants.view);
	glm::mat4 model{1.f};
	memcpy(&constants.model, &model[0][0], sizeof constants.model);
	m_d3dCommandList->SetGraphicsRoot32BitConstants(0, sizeof constants / 4,
													&constants, 0);

	// Draw the vertices:
	m_d3dCommandList->IASetPrimitiveTopology(
		::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
	m_d3dCommandList->DrawInstanced(m_d3dVertexBufferView.SizeInBytes /
										m_d3dVertexBufferView.StrideInBytes,
									1, 0, 0);

	// Begin present:
	barrier.Transition.StateBefore = ::D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = ::D3D12_RESOURCE_STATE_PRESENT;
	m_d3dCommandList->ResourceBarrier(1, &barrier);

	DXRASSERT(DXRSUCCESSTEST(m_d3dCommandList->Close()));

	::ID3D12CommandList* commandLists[] = {m_d3dCommandList.Get()};
	m_d3dCommandQueue->ExecuteCommandLists(1, commandLists);
}