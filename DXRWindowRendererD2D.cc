#include "DXRWindowRenderer.h"

#ifndef DXRDISABLED2D

auto DXRWindowRenderer::CreateD3D11On12DeviceAndContext() -> bool
{
	if (!m_d3d11Device || !m_d3d11DeviceContext || !m_d3d11On12Device)
	{
		m_d3d11Device = nullptr;
		m_d3d11DeviceContext = nullptr;
		m_d3d11On12Device = nullptr;

		UINT flags{::D3D11_CREATE_DEVICE_BGRA_SUPPORT};
#ifdef _DEBUG
		flags |= ::D3D11_CREATE_DEVICE_DEBUG;
#endif
		::ID3D12CommandQueue* commandQueues[] = {m_d3dCommandQueue.Get()};
		if (DXRSUCCESSTEST(::D3D11On12CreateDevice(
				m_d3dDevice.Get(), flags, nullptr, 0,
				reinterpret_cast<IUnknown**>(commandQueues), 1, 0,
				m_d3d11Device.Out(), m_d3d11DeviceContext.Out(), nullptr)))
		{
			DXRASSERT(m_d3d11Device);
			DXRASSERT(m_d3d11DeviceContext);

			if (DXRSUCCESSTEST(m_d3d11Device->QueryInterface(
					m_d3d11On12Device.static_uuid, m_d3d11On12Device.Out())))
			{
				DXRASSERT(m_d3d11On12Device);
				return m_d3d11Device && m_d3d11DeviceContext &&
					   m_d3d11On12Device;
			}
		}
	}
	return m_d3d11Device && m_d3d11DeviceContext && m_d3d11On12Device;
}

auto DXRWindowRenderer::CreateD2DFactory() -> bool
{
	if (!m_d2dFactory)
	{
		D2D1_FACTORY_OPTIONS options{};
#ifdef _DEBUG
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
		if (DXRSUCCESSTEST(::D2D1CreateFactory(
				::D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.static_uuid,
				&options, m_d2dFactory.Out())))
		{
			DXRASSERT(m_d2dFactory);
			return m_d2dFactory;
		}
	}
	return m_d2dFactory;
}

auto DXRWindowRenderer::CreateD2DDeviceAndContext() -> bool
{
	if (!m_d2dDevice)
	{
		COMPtr<::IDXGIDevice> dxgiDevice;
		if (DXRSUCCESSTEST(m_d3d11On12Device->QueryInterface(
				dxgiDevice.static_uuid, dxgiDevice.Out())))
		{
			DXRASSERT(dxgiDevice);
			if (DXRSUCCESSTEST(m_d2dFactory->CreateDevice(dxgiDevice.Get(),
														  m_d2dDevice.Out())))
			{
				DXRASSERT(m_d2dDevice);
			}
		}
	}
	if (!m_d2dDevice)
		return false;

	if (!m_d2dDeviceContext)
	{
		if (DXRSUCCESSTEST(m_d2dDevice->CreateDeviceContext(
				::D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dDeviceContext.Out())))
		{
			DXRASSERT(m_d2dDeviceContext);
		}
	}
	return m_d2dDeviceContext;
}

auto DXRWindowRenderer::CreateD2DBrushes() -> bool
{
	if (!m_d2dSolidColorBrush)
	{
		DXRASSERT(DXRSUCCESSTEST(m_d2dDeviceContext->CreateSolidColorBrush(
			::D2D1::ColorF(::D2D1::ColorF::Black),
			m_d2dSolidColorBrush.Out())));
	}
	return m_d2dSolidColorBrush;
}

auto DXRWindowRenderer::CreateD2D1RenderTargets() -> bool
{
	for (UINT n{}; n < k_NumSwapChainBuffers; n++)
	{
		DXRASSERT(m_d3dRenderTargets[n]);
		if (!m_d3d11WrappedRenderTargets[n])
		{
			::D3D11_RESOURCE_FLAGS flags{::D3D11_BIND_RENDER_TARGET};
			DXRASSERT(DXRSUCCESSTEST(m_d3d11On12Device->CreateWrappedResource(
				m_d3dRenderTargets[n].Get(), &flags,
				::D3D12_RESOURCE_STATE_RENDER_TARGET,
				::D3D12_RESOURCE_STATE_PRESENT,
				m_d3d11WrappedRenderTargets[n].static_uuid,
				m_d3d11WrappedRenderTargets[n].Out())));
			DXRASSERT(m_d3d11WrappedRenderTargets[n]);
		}

		if (!m_d2dRenderTargets[n])
		{
			COMPtr<::IDXGISurface> surf{};
			DXRASSERT(
				DXRSUCCESSTEST(m_d3d11WrappedRenderTargets[n]->QueryInterface(
					surf.static_uuid, surf.Out())));
			DXRASSERT(surf);

			::D2D1_BITMAP_PROPERTIES1 bitmapProperties =
				D2D1::BitmapProperties1(
					::D2D1_BITMAP_OPTIONS_TARGET |
						::D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
					D2D1::PixelFormat(::DXGI_FORMAT_UNKNOWN,
									  ::D2D1_ALPHA_MODE_PREMULTIPLIED));
			DXRASSERT(
				DXRSUCCESSTEST(m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
					surf.Get(), &bitmapProperties,
					m_d2dRenderTargets[n].Out())));
		}
	}
	return true;
}

auto DXRWindowRenderer::SubmitD2D() -> void
{
	const auto d2dRenderTarget =
		m_d3d11WrappedRenderTargets[m_frameIndex].Get();
	m_d3d11On12Device->AcquireWrappedResources(&d2dRenderTarget, 1);

	m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[m_frameIndex].Get());

	m_d2dDeviceContext->BeginDraw();

	static int idx = 1;

	float v = static_cast<float>(idx) / 1000.0f;
	if (++idx > 1000)
	{
		idx = 1;
	}
		

	m_d2dDeviceContext->DrawLine(
		::D2D1::Point2F(0.0f, 0.0f),
		::D2D1::Point2F(static_cast<float>(m_width) * v,
						static_cast<float>(m_height) * v),
		m_d2dSolidColorBrush.Get());

	DXRASSERT(DXRSUCCESSTEST(m_d2dDeviceContext->EndDraw()));

	m_d3d11On12Device->ReleaseWrappedResources(&d2dRenderTarget, 1);

	m_d3d11DeviceContext->Flush();
}

#endif