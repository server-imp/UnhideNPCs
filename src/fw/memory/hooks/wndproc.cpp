#include "wndproc.hpp"

memory::hooks::wndproc* memory::hooks::wndproc::_instance {};

memory::hooks::wndproc::wndproc(HWND hWnd) : hook("WndProc", nullptr, nullptr, nullptr)
{
    _instance = this;
    _hWnd     = hWnd;
}

memory::hooks::wndproc::~wndproc()
{
    disable(false);
    if (_instance == this)
        _instance = nullptr;
}

bool memory::hooks::wndproc::enable()
{
    _originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
        _hWnd,
        GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(WndProc)
    ));

    if (!_originalWndProc)
        return false;

    return true;
}

bool memory::hooks::wndproc::disable(bool uninitialize)
{
    SetWindowLongPtr(_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_originalWndProc));

    return true;
}

void memory::hooks::wndproc::addCallback(const std::function<bool(HWND, UINT, WPARAM, LPARAM)>& callback)
{
    _callbacks.push_back(callback);
}

LRESULT memory::hooks::wndproc::InternalWndProc(HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    bool callOriginal = true;

    for (const auto& callback : _callbacks)
    {
        if (callback(hWnd, msg, wParam, lParam))
            callOriginal = false;
    }

    if (!callOriginal)
        return 0;

    return CallWindowProc(_originalWndProc, hWnd, msg, wParam, lParam);
}

LRESULT memory::hooks::wndproc::WndProc(HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    if (!_instance)
        return 0;

    return _instance->InternalWndProc(hWnd, msg, wParam, lParam);
}
