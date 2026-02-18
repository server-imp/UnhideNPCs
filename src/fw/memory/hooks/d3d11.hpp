#ifndef UNHIDENPCS_D3D11_HPP
#define UNHIDENPCS_D3D11_HPP
#pragma once
#include <d3d11.h>

#include "fw/memory/hook.hpp"

namespace memory::hooks
{
    class D3D11 final : Hook
    {
    private:
        static D3D11* _instance;

        std::function<void()>             _cbPresent {};
        std::function<void(D3D11*, bool)> _cbResizeBuffers {};
        std::function<bool(D3D11*)>       _cbStarted {};
        std::function<void(D3D11*)>       _cbShutdown {};

        std::optional<Detour> _hkPresent {};
        std::optional<Detour> _hkResizeBuffers {};

        void* _presentPtr {};
        void* _resizeBuffersPtr {};

        ID3D11Device*           _pDevice {};
        ID3D11DeviceContext*    _pContext {};
        ID3D11RenderTargetView* _pRenderTargetView {};

        HWND _hWnd {};

        std::mutex _mutex;

        bool _shuttingDown {};

        D3D11(
            HWND                                     hWnd,
            void*                                    presentPtr,
            void*                                    resizeBuffersPtr,
            const std::function<void()>&             cbPresent,
            const std::function<void(D3D11*, bool)>& cbResizeBuffers,
            const std::function<bool(D3D11*)>&       cbStarted,
            const std::function<void(D3D11*)>&       cbShutdown
        );

    public:
        ~D3D11();

        virtual bool enable();

        virtual bool disable(bool uninitialize);

        [[nodiscard]] ID3D11Device* device() const;

        [[nodiscard]] ID3D11DeviceContext* context() const;

        [[nodiscard]] ID3D11RenderTargetView* renderTargetView() const;

        [[nodiscard]] HWND hWnd() const;

        static std::optional<std::unique_ptr<D3D11>> create(
            const std::string&                       windowClassName,
            const std::string&                       windowName,
            const std::function<void()>&             cbPresent,
            const std::function<void(D3D11*, bool)>& cbResizeBuffers,
            const std::function<bool(D3D11*)>&       cbStarted,
            const std::function<void(D3D11*)>&       cbShutdown
        );

    private:
        bool createRenderTarget(IDXGISwapChain* swapChain);

        void destroyRenderTarget();

        HRESULT internalPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);

        HRESULT internalResizeBuffers(
            IDXGISwapChain* swapChain,
            UINT            bufferCount,
            UINT            width,
            UINT            height,
            DXGI_FORMAT     newFormat,
            UINT            swapChainFlags
        );

        static HRESULT present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);

        static HRESULT resizeBuffers(
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
