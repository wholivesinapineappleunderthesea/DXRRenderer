#pragma once

#include "W32Platform.h"

#include <string>
#include <any>


struct W32Window
{
	inline W32Window() = default;
	inline ~W32Window()
	{
		Destroy();
	}

	inline W32Window(const W32Window&) = delete;
	inline W32Window(W32Window&&) = delete;
	inline W32Window& operator=(const W32Window&) = delete;
	inline W32Window& operator=(W32Window&&) = delete;

	// Validation.
	auto IsValid() const -> bool;

	// Native HWND.
	auto NativeHandle() const -> NTNamespace::HWND;

	// Creation and destruction.
	auto Create() -> bool;
	auto Destroy() -> bool;

	// These may be invoked before or after Create().
	auto SetPosition(int x, int y) -> bool;
	auto SetSize(int w, int h) -> bool;
	auto SetStyle(NTNamespace::LONG style) -> bool;
	auto SetExStyle(NTNamespace::DWORD exStyle) -> bool;
	auto SetMenu(NTNamespace::HMENU menu) -> bool;
	auto SetParent(NTNamespace::HWND hwnd) -> bool;
	auto SetTitle(NTNamespace::LPCWSTR name) -> bool;
	auto SetClassName(NTNamespace::LPCWSTR name) -> bool;
	// Will bypass the default window procedure from the class.
	// This should only be invoked after Create().
	auto SetWndProcOverride(NTNamespace::WNDPROC wndproc) -> bool;
	auto GetWndProcHandler() const -> NTNamespace::WNDPROC;

	template <typename T> inline auto SetUserData(T&& data) -> void
	{
		m_userData = std::forward<T>(data);
	}

	inline auto GetUserData() -> std::any&
	{
		return m_userData;
	}

	static auto GetFromHWND(NTNamespace::HWND hwnd) -> W32Window*;

	// Visibility.
	auto Show(bool yes = true) -> bool;

	// Update.
	auto Update() -> bool;

	// These should be invoked BEFORE Create().
	auto SetClassBackground(NTNamespace::HBRUSH brush) -> bool;
	auto SetClassCursor(NTNamespace::HCURSOR cursor) -> bool;
	auto SetClassIcon(NTNamespace::HICON icon) -> bool;
	auto SetClassIconSm(NTNamespace::HICON icon) -> bool;
	auto SetClassMenuName(NTNamespace::LPCWSTR name) -> bool;
	auto SetClassStyle(NTNamespace::UINT style) -> bool;
	auto SetClassWindowProc(NTNamespace::WNDPROC wndproc) -> bool;

	// Messages.
	// Will return false if WM_QUIT was received.
	static auto DispatchMessagesThisThread() -> bool;

private:
	std::wstring m_title{L"DXRDefaultWindowName"};
	std::wstring m_className{L"DXRDefaultWindowClass"};
	NTNamespace::HWND m_hwnd{};
	NTNamespace::DWORD m_windowThreadID{};
	int m_x{CW_USEDEFAULT}, m_y{CW_USEDEFAULT};
	int m_w{CW_USEDEFAULT}, m_h{CW_USEDEFAULT};
	NTNamespace::LONG m_style{WS_OVERLAPPEDWINDOW};
	NTNamespace::DWORD m_exStyle{WS_EX_APPWINDOW};
	NTNamespace::HWND m_parent{};
	NTNamespace::HMENU m_menu{};

	NTNamespace::WNDPROC m_wndprocOverride{};

	NTNamespace::UINT m_classStyle{CS_HREDRAW | CS_VREDRAW};
	NTNamespace::HBRUSH m_classBackground{};
	NTNamespace::HCURSOR m_classCursor{};
	NTNamespace::HICON m_classIcon{};
	NTNamespace::HICON m_classIconSm{};
	std::wstring m_classMenuName{};
	NTNamespace::WNDPROC m_classWndProc{DefWindowProcW};

	std::any m_userData{};
};