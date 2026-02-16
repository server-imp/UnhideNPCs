#ifndef UNHIDENPCS_D3D11_HPP
#define UNHIDENPCS_D3D11_HPP
#pragma once
#include <d3d11.h>

#include "fw/memory/hook.hpp"

namespace memory::hooks
{
    class d3d11 final : hook
    {
    private:
        static d3d11* _instance;

        std::function<void()>             _cbPresent {};
        std::function<void(d3d11*, bool)> _cbResizeBuffers {};
        std::function<bool(d3d11*)>       _cbStarted {};
        std::function<void(d3d11*)>       _cbShutdown {};

        std::optional<detour> _hkPresent {};
        std::optional<detour> _hkResizeBuffers {};

        void* _presentPtr {};
        void* _resizeBuffersPtr {};

        ID3D11Device*           _pDevice {};
        ID3D11DeviceContext*    _pContext {};
        ID3D11RenderTargetView* _pRenderTargetView {};

        HWND _hWnd {};

        std::mutex _mutex;

        bool _shuttingDown {};

        d3d11(
            HWND                                     hWnd,
            void*                                    presentPtr,
            void*                                    resizeBuffersPtr,
            const std::function<void()>&             cbPresent,
            const std::function<void(d3d11*, bool)>& cbResizeBuffers,
            const std::function<bool(d3d11*)>&       cbStarted,
            const std::function<void(d3d11*)>&       cbShutdown
        );

    public:
        ~d3d11();

        virtual bool enable();

        virtual bool disable(bool uninitialize);

        [[nodiscard]] ID3D11Device* device() const;

        [[nodiscard]] ID3D11DeviceContext* context() const;

        [[nodiscard]] ID3D11RenderTargetView* renderTargetView() const;

        [[nodiscard]] HWND hWnd() const;

        static std::optional<std::unique_ptr<d3d11>> create(
            const std::string&                       windowClassName,
            const std::string&                       windowName,
            const std::function<void()>&             cbPresent,
            const std::function<void(d3d11*, bool)>& cbResizeBuffers,
            const std::function<bool(d3d11*)>&       cbStarted,
            const std::function<void(d3d11*)>&       cbShutdown
        );

    private:
        bool CreateRenderTarget(IDXGISwapChain* swapChain);

        void DestroyRenderTarget();

        HRESULT InternalPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);

        HRESULT InternalResizeBuffers(
            IDXGISwapChain* swapChain,
            UINT            bufferCount,
            UINT            width,
            UINT            height,
            DXGI_FORMAT     newFormat,
            UINT            swapChainFlags
        );

        static HRESULT Present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);

        static HRESULT ResizeBuffers(
            IDXGISwapChain* swapChain,
            UINT            bufferCount,
            UINT            width,
            UINT            height,
            DXGI_FORMAT     newFormat,
            UINT            swapChainFlags
        );
    };
}

#endif //UNHIDENPCS_D3D11_HPP
