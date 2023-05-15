#pragma once

#include "W32Platform.h"
#include "DXRWindowRenderer.h"

#include <cassert>

struct DXRRenderer
{
	inline DXRRenderer()
	{
		m_window = std::make_unique<W32Window>();
		assert(m_window);

		m_window->SetUserData(this);
		m_window->SetClassName(L"DXRClass");
		m_window->SetClassStyle(CS_HREDRAW | CS_VREDRAW);
		m_window->SetClassWindowProc([](NTNamespace::HWND hwnd, NTNamespace::UINT msg,
			NTNamespace::WPARAM wparam,
			NTNamespace::LPARAM lparam) -> NTNamespace::LRESULT {
			const auto wnd = W32Window::GetFromHWND(hwnd);
			const auto dxr = std::any_cast<DXRRenderer*>(wnd->GetUserData());
			switch (msg)
			{
			case WM_DESTROY:
				return true;
			case WM_SIZE: {
				if (wparam != SIZE_MINIMIZED)
				{
					NTNamespace::UINT w = LOWORD(lparam);
					NTNamespace::UINT h = HIWORD(lparam);
					dxr->OnResize(w, h);
				}
				return 0;
			}
			case WM_CLOSE:
				::PostQuitMessage(0);
				dxr->Destroy();

				return true;
			default:
				return NTNamespace::DefWindowProcW(hwnd, msg, wparam, lparam);
			}
		});
		m_window->SetClassBackground(
			reinterpret_cast<NTNamespace::HBRUSH>(NTNamespace::GetStockObject(WHITE_BRUSH)));
		m_window->SetClassCursor(NTNamespace::LoadCursorA(0, IDC_ARROW));
		m_window->SetClassIcon(NTNamespace::LoadIconA(0, IDI_APPLICATION));
		m_window->SetClassIconSm(NTNamespace::LoadIconA(0, IDI_APPLICATION));
		m_window->SetClassMenuName(0);

		m_window->SetTitle(L"DXRWindow");
		m_window->SetStyle(WS_OVERLAPPEDWINDOW);
		m_window->SetExStyle(0);
		m_window->SetPosition(0, 0);
		m_window->SetSize(640, 480);
		m_window->SetMenu(0);
		m_window->SetParent(0);

		if (!m_window->Create())
		{
			// Something bad, real bad.
			assert(0);
		}

		m_window->Show();
		m_window->Update();

		m_renderer = std::make_unique<DXRWindowRenderer>(m_window.get());
		assert(m_renderer);
		if (!m_renderer->ValidateAndCreateObjects())
		{
			assert(0);
		}
	}

	inline ~DXRRenderer()
	{
	}

	inline auto Destroy() -> void
	{
		// The order is important here, the renderer has a raw pointer to the
		// window.
		m_renderer.reset();
		m_window.reset();
	}

	inline auto IsValid() -> bool
	{
		if (!m_window || !m_renderer)
		{
			return false;
		}

		if (!m_window->IsValid())
		{
			return false;
		}

		return true;
	}

	inline auto OnResize(NTNamespace::UINT w, NTNamespace::UINT h) -> void
	{
		if (m_renderer)
		{
			m_renderer->OnResize(w, h);
		}
	}

	inline auto Render() -> bool
	{
		if (!IsValid())
			return false;

		m_renderer->RenderAll();
		return true;
	}

	inline auto Update(float dt) -> void
	{
		if (!IsValid())
			return;
		m_renderer->Update(dt);
	}

  private:
	std::unique_ptr<W32Window> m_window{};
	std::unique_ptr<DXRWindowRenderer> m_renderer{};
};