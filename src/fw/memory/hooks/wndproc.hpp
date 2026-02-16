#ifndef UNHIDENPCS_WNDPROC_HPP
#define UNHIDENPCS_WNDPROC_HPP
#pragma once
#include "fw/memory/hook.hpp"

namespace memory::hooks
{
    class wndproc final : hook
    {
    private:
        static wndproc* _instance;
        HWND            _hWnd {};
        WNDPROC         _originalWndProc {};

        std::vector<std::function<bool(HWND, UINT, WPARAM, LPARAM)>> _callbacks {};

    public:
        explicit wndproc(HWND hWnd);

        ~wndproc();

        virtual bool enable();

        virtual bool disable(bool uninitialize);

        void addCallback(const std::function<bool(HWND, UINT, WPARAM, LPARAM)>& callback);

    private:
        LRESULT CALLBACK InternalWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

        static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    public:
    };
}

#endif //UNHIDENPCS_WNDPROC_HPP
