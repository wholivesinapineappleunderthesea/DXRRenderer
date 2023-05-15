#include "W32Window.h"

#include <cassert>

auto W32Window::IsValid() const -> bool
{
	return m_hwnd;
}

auto W32Window::NativeHandle() const -> NTNamespace::HWND
{
	return m_hwnd;
}

auto W32Window::Create() -> bool
{
	if (IsValid())
	{
		return false;
	}

	m_windowThreadID = ::GetCurrentThreadId();

	NTNamespace::WNDCLASSEXW cls{};
	cls.cbSize = sizeof cls;
	cls.style = m_classStyle;
	// cls.lpfnWndProc = wndprocOverride ? wndprocOverride : m_classWndProc;
	cls.lpfnWndProc = [](NTNamespace::HWND hwnd, NTNamespace::UINT msg, NTNamespace::WPARAM wparam,
		NTNamespace::LPARAM lparam) -> NTNamespace::LRESULT {
			if (msg == WM_CREATE)
			{
				const auto createStruct =
					reinterpret_cast<NTNamespace::CREATESTRUCTW*>(lparam);
				const auto window =
					reinterpret_cast<W32Window*>(createStruct->lpCreateParams);
				// Override the USERDATA window attribute with our this
				::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
					reinterpret_cast<NTNamespace::LONG_PTR>(window));
				// Get the WNDPROC the user actually wants to execute.
				const auto wproc = window->GetWndProcHandler();
				// Set it so that this lambda never gets called again.
				::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
					reinterpret_cast<NTNamespace::LONG_PTR>(wproc));
				// Send us to the user's WNDPROC.
				return wproc(hwnd, msg, wparam, lparam);
			}
			return NTNamespace::DefWindowProcW(hwnd, msg, wparam, lparam);
	};
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = NTNamespace::GetModuleHandleW(0);
	cls.hIcon = m_classIcon;
	cls.hCursor = m_classCursor;
	cls.hbrBackground = m_classBackground;
	cls.lpszMenuName = m_classMenuName.c_str();
	cls.lpszClassName = m_className.c_str();
	cls.hIconSm = m_classIconSm;

	if (!NTNamespace::RegisterClassExW(&cls))
	{
		return false;
	}

	m_hwnd = NTNamespace::CreateWindowExW(m_exStyle, m_className.c_str(), m_title.c_str(),
		m_style, m_x, m_y, m_w, m_h, m_parent, m_menu,
		NTNamespace::GetModuleHandleW(0), this);

	return IsValid();
}

auto W32Window::Destroy() -> bool
{
	bool fail = false;

	if (IsValid())
	{
		if (!NTNamespace::DestroyWindow(m_hwnd))
		{
			fail = true;
		}
		m_hwnd = 0;
	}

	if (!m_className.empty())
	{
		if (!NTNamespace::UnregisterClassW(m_className.c_str(), 0))
		{
			fail = true;
		}
	}

	return fail;
}

auto W32Window::SetPosition(int x, int y) -> bool
{

	if (!IsValid())
	{
		m_x = x;
		m_y = y;
		return true;
	}

	if (!NTNamespace::SetWindowPos(m_hwnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE))
	{
		return false;
	}

	m_x = x;
	m_y = y;
	return true;
}

auto W32Window::SetSize(int w, int h) -> bool
{
	if (!IsValid())
	{
		m_w = w;
		m_h = h;
		return true;
	}

	if (!NTNamespace::SetWindowPos(m_hwnd, 0, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE))
	{
		return false;
	}

	m_w = w;
	m_h = h;

	return true;
}

auto W32Window::SetStyle(::LONG style) -> bool
{
	if (!IsValid())
	{
		m_style = style;
		return true;
	}

	if (!NTNamespace::SetWindowLongW(NativeHandle(), GWL_STYLE, style))
	{
		return false;
	}

	m_style = style;

	return false;
}

auto W32Window::SetExStyle(NTNamespace::DWORD exStyle) -> bool
{
	if (!IsValid())
	{
		m_exStyle = exStyle;
		return true;
	}

	if (!NTNamespace::SetWindowLongW(NativeHandle(), GWL_EXSTYLE, exStyle))
	{
		return false;
	}

	m_exStyle = exStyle;

	return false;
}

auto W32Window::SetMenu(NTNamespace::HMENU menu) -> bool
{
	if (!IsValid())
	{
		m_menu = menu;
		return true;
	}

	if (!NTNamespace::SetMenu(NativeHandle(), menu))
	{
		return false;
	}
	m_menu = menu;
	return false;
}

auto W32Window::SetParent(NTNamespace::HWND hwnd) -> bool
{
	if (!IsValid())
	{
		m_parent = hwnd;
		return true;
	}
	if (!NTNamespace::SetParent(NativeHandle(), hwnd))
	{
		return false;
	}
	m_parent = hwnd;
	return true;
}

auto W32Window::SetTitle(NTNamespace::LPCWSTR name) -> bool
{
	m_title.clear();

	if (!IsValid())
	{
		if (name)
			m_title = name;
		return true;
	}
	if (name)
	{
		if (!NTNamespace::SetWindowTextW(NativeHandle(), name))
		{
			return false;
		}
		m_title = name;
		return true;
	}
	return false;
}

auto W32Window::SetClassName(NTNamespace::LPCWSTR name) -> bool
{
	if (IsValid())
		return false;
	m_className = name;
	return true;
}

auto W32Window::SetWndProcOverride(NTNamespace::WNDPROC wndproc) -> bool
{
	assert(wndproc);
	if (IsValid())
	{
		m_wndprocOverride = wndproc;
		return false;
	}
	m_wndprocOverride = wndproc;
	return NTNamespace::SetWindowLongPtrW(NativeHandle(), GWLP_WNDPROC,
		reinterpret_cast<NTNamespace::LONG_PTR>(wndproc)) != 0;
}

auto W32Window::GetWndProcHandler() const -> NTNamespace::WNDPROC
{
	return m_wndprocOverride ? m_wndprocOverride : m_classWndProc;
}

auto W32Window::GetFromHWND(NTNamespace::HWND hwnd) -> W32Window*
{
	const auto wnd = NTNamespace::GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	return reinterpret_cast<W32Window*>(wnd);
}

auto W32Window::Show(bool yes) -> bool
{
	if (!IsValid())
		return false;
	return NTNamespace::ShowWindow(NativeHandle(), yes ? SW_SHOW : SW_HIDE) != 0;
}

auto W32Window::Update() -> bool
{
	if (!IsValid())
		return false;
	return NTNamespace::UpdateWindow(NativeHandle());
}

auto W32Window::SetClassBackground(NTNamespace::HBRUSH brush) -> bool
{
	m_classBackground = brush;
	return true;
}

auto W32Window::SetClassCursor(NTNamespace::HCURSOR cursor) -> bool
{
	m_classCursor = cursor;
	return true;
}

auto W32Window::SetClassIcon(NTNamespace::HICON icon) -> bool
{
	m_classIcon = icon;
	return true;
}

auto W32Window::SetClassIconSm(NTNamespace::HICON icon) -> bool
{
	m_classIconSm = icon;
	return true;
}

auto W32Window::SetClassMenuName(NTNamespace::LPCWSTR name) -> bool
{
	m_classMenuName.clear();
	if (name)
		m_classMenuName = name;
	return true;
}

auto W32Window::SetClassStyle(NTNamespace::UINT style) -> bool
{
	m_classStyle = style;
	return true;
}

auto W32Window::SetClassWindowProc(::WNDPROC wndproc) -> bool
{
	assert(wndproc);
	m_classWndProc = wndproc;
	return true;
}

auto W32Window::DispatchMessagesThisThread() -> bool
{
	NTNamespace::MSG msg{};
	while (NTNamespace::PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}
		NTNamespace::TranslateMessage(&msg);
		NTNamespace::DispatchMessageW(&msg);
	}
	return true;
}