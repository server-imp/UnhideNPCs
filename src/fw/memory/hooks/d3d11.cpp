#include "d3d11.hpp"
#include <d3d11.h>
#include "fw/logger.hpp"

memory::hooks::D3D11* memory::hooks::D3D11::_instance = nullptr;

template <typename T>
static void safeRelease(T*& p)
{
    if (!p)
    {
        return;
    }

    p->Release();
    p = nullptr;
}

memory::hooks::D3D11::D3D11(
    HWND                                     hWnd,
    void*                                    presentPtr,
    void*                                    resizeBuffersPtr,
    const std::function<void()>&             cbPresent,
    const std::function<void(D3D11*, bool)>& cbResizeBuffers,
    const std::function<bool(D3D11*)>&       cbStarted,
    const std::function<void(D3D11*)>&       cbShutdown
) : Hook("D3D11", nullptr, nullptr, nullptr), _hWnd(hWnd), _cbPresent(cbPresent), _cbResizeBuffers(cbResizeBuffers),
    _cbStarted(cbStarted), _cbShutdown(cbShutdown), _presentPtr(presentPtr), _resizeBuffersPtr(resizeBuffersPtr)
{
    _instance = this;

    _hkPresent.emplace("D3D11Present", presentPtr, reinterpret_cast<void*>(present));
    _hkResizeBuffers.emplace("D3D11ResizeBuffers", resizeBuffersPtr, reinterpret_cast<void*>(resizeBuffers));
}

memory::hooks::D3D11::~D3D11()
{
    disable(false);
    if (_instance == this)
    {
        _instance = nullptr;
    }
}

bool memory::hooks::D3D11::enable()
{
    _shuttingDown = false;

    if (!_hkPresent || !_hkPresent->enable())
    {
        return false;
    }
    if (!_hkResizeBuffers || !_hkResizeBuffers->enable())
    {
        _hkPresent->disable(false);
        return false;
    }

    _enabled = true;
    return true;
}

bool memory::hooks::D3D11::disable(bool uninitialize)
{
    _shuttingDown.store(true, std::memory_order_release);

    if (_cbShutdown)
    {
        _cbShutdown(this);
    }

    if (_hkPresent->enabled())
    {
        _hkPresent->disable(false);
        while (_presentInFlight.load(std::memory_order_acquire) != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    if (_hkResizeBuffers->enabled())
    {
        _hkResizeBuffers->disable(false);
        while (_resizeBuffersInFlight.load(std::memory_order_acquire) != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    destroyRenderTarget();
    safeRelease(_pContext);
    safeRelease(_pDevice);

    _enabled = false;
    return true;
}

ID3D11Device* memory::hooks::D3D11::device() const
{
    return _pDevice;
}

ID3D11DeviceContext* memory::hooks::D3D11::context() const
{
    return _pContext;
}

ID3D11RenderTargetView* memory::hooks::D3D11::renderTargetView() const
{
    return _pRenderTargetView;
}

HWND memory::hooks::D3D11::hWnd() const
{
    return _hWnd;
}

std::optional<std::unique_ptr<memory::hooks::D3D11>> memory::hooks::D3D11::create(
    const std::string&                       windowClassName,
    const std::string&                       windowName,
    const std::function<void()>&             cbPresent,
    const std::function<void(D3D11*, bool)>& cbResizeBuffers,
    const std::function<bool(D3D11*)>&       cbStarted,
    const std::function<void(D3D11*)>&       cbShutdown
)
{
    if (_instance)
    {
        LOG_ERR("D3D11 hook already initialized");
        return std::nullopt;
    }

    auto hWnd = FindWindow(
        windowClassName.empty() ? nullptr : windowClassName.c_str(),
        windowName.empty() ? nullptr : windowName.c_str()
    );
    if (!hWnd)
    {
        LOG_ERR("FindWindow failed: [{}], \"{}\", \"{}\"", GetLastError(), windowClassName, windowName);
        return std::nullopt;
    }

    static void* _presentPtr {};
    static void* _resizeBuffersPtr {};

    if (!_presentPtr && !_resizeBuffersPtr)
    {
        WNDCLASSEXA wc {};
        wc.cbSize        = sizeof(WNDCLASSEXA);
        wc.lpfnWndProc   = DefWindowProcA;
        wc.hInstance     = GetModuleHandle(nullptr);
        wc.lpszClassName = "D3D11DummyWindowClass";

        if (!RegisterClassEx(&wc))
        {
            LOG_ERR("Failed to register dummy window class");
            return std::nullopt;
        }

        HWND dummyWnd = CreateWindowEx(
            0,
            wc.lpszClassName,
            "D3D11DummyWindow",
            WS_OVERLAPPEDWINDOW,
            0,
            0,
            100,
            100,
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr
        );

        if (!dummyWnd)
        {
            LOG_ERR("Failed to create dummy window");
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            return std::nullopt;
        }

        DXGI_SWAP_CHAIN_DESC sd {};
        sd.BufferCount       = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow      = dummyWnd;
        sd.SampleDesc.Count  = 1;
        sd.Windowed          = TRUE;
        sd.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

        constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1 };

        IDXGISwapChain*      tempSwap = nullptr;
        ID3D11Device*        tempDev  = nullptr;
        ID3D11DeviceContext* tempCtx  = nullptr;

        const auto hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            levels,
            std::size(levels),
            D3D11_SDK_VERSION,
            &sd,
            &tempSwap,
            &tempDev,
            nullptr,
            &tempCtx
        );

        if (FAILED(hr) || !tempSwap)
        {
            LOG_ERR("Dummy device creation failed: {:08X}", static_cast<unsigned>(hr));
            safeRelease(tempCtx);
            safeRelease(tempDev);
            safeRelease(tempSwap);
            DestroyWindow(dummyWnd);
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            return std::nullopt;
        }

        void** vmt = *reinterpret_cast<void***>(tempSwap);
        if (!vmt)
        {
            LOG_ERR("Failed to get VMT");
            safeRelease(tempCtx);
            safeRelease(tempDev);
            safeRelease(tempSwap);
            DestroyWindow(dummyWnd);
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            return std::nullopt;
        }

        _presentPtr       = vmt[8];
        _resizeBuffersPtr = vmt[13];
        LOG_DBG("Present: {:p}, ResizeBuffers: {:p}", _presentPtr, _resizeBuffersPtr);

        safeRelease(tempCtx);
        safeRelease(tempDev);
        safeRelease(tempSwap);

        DestroyWindow(dummyWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
    }

    return std::unique_ptr<memory::hooks::D3D11>(
        new memory::hooks::D3D11(
            hWnd,
            _presentPtr,
            _resizeBuffersPtr,
            cbPresent,
            cbResizeBuffers,
            cbStarted,
            cbShutdown
        )
    );
}

bool memory::hooks::D3D11::createRenderTarget(IDXGISwapChain* swapChain)
{
    std::lock_guard lock(_mutex);

    safeRelease(_pRenderTargetView);

    ID3D11Texture2D* backBuffer = nullptr;
    if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer))))
    {
        LOG_ERR("GetBuffer failed");
        return false;
    }

    const auto hr = _pDevice->CreateRenderTargetView(backBuffer, nullptr, &_pRenderTargetView);
    backBuffer->Release();

    if (FAILED(hr) || !_pRenderTargetView)
    {
        LOG_ERR("CreateRenderTargetView");
        safeRelease(_pRenderTargetView);
        return false;
    }

    D3D11_VIEWPORT vp {};
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    DXGI_SWAP_CHAIN_DESC sd {};
    if (SUCCEEDED(swapChain->GetDesc(&sd)))
    {
        vp.Width  = static_cast<float>(sd.BufferDesc.Width);
        vp.Height = static_cast<float>(sd.BufferDesc.Height);
    }
    else
    {
        D3D11_TEXTURE2D_DESC td {};
        ID3D11Texture2D*     tex = nullptr;
        if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))))
        {
            tex->GetDesc(&td);
            vp.Width  = static_cast<float>(td.Width);
            vp.Height = static_cast<float>(td.Height);
            tex->Release();
        }
    }

    _pContext->OMSetRenderTargets(1, &_pRenderTargetView, nullptr);
    _pContext->RSSetViewports(1, &vp);

    return true;
}

void memory::hooks::D3D11::destroyRenderTarget()
{
    std::lock_guard lock(_mutex);
    safeRelease(_pRenderTargetView);
}

HRESULT memory::hooks::D3D11::internalPresent(IDXGISwapChain* swapChain, const UINT syncInterval, const UINT flags)
{
    HookScope scope(_presentInFlight);

    if (_shuttingDown.load(std::memory_order_acquire))
    {
        return _hkPresent->original<decltype(&present)>()(swapChain, syncInterval, flags);
    }

    if (!_pDevice)
    {
        if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&_pDevice))))
        {
            LOG_ERR("Failed to get device");
            return _hkPresent->original<decltype(&present)>()(swapChain, syncInterval, flags);
        }

        _pDevice->GetImmediateContext(&_pContext);

        if (!createRenderTarget(swapChain))
        {
            safeRelease(_pContext);
            safeRelease(_pDevice);
            return _hkPresent->original<decltype(&present)>()(swapChain, syncInterval, flags);
        }

        if (_cbStarted && !_cbStarted(this))
        {
            destroyRenderTarget();
            safeRelease(_pContext);
            safeRelease(_pDevice);
            return _hkPresent->original<decltype(&present)>()(swapChain, syncInterval, flags);
        }
    }

    if (_cbPresent)
    {
        _pContext->OMSetRenderTargets(1, &_pRenderTargetView, nullptr);
        _cbPresent();
    }

    return _hkPresent->original<decltype(&present)>()(swapChain, syncInterval, flags);
}

HRESULT memory::hooks::D3D11::internalResizeBuffers(
    IDXGISwapChain*   swapChain,
    const UINT        bufferCount,
    const UINT        width,
    const UINT        height,
    const DXGI_FORMAT newFormat,
    const UINT        swapChainFlags
)
{
    HookScope scope(_resizeBuffersInFlight);

    if (_shuttingDown.load(std::memory_order_acquire))
    {
        return _hkResizeBuffers->original<decltype(&resizeBuffers)>()(
            swapChain,
            bufferCount,
            width,
            height,
            newFormat,
            swapChainFlags
        );
    }

    if (_cbResizeBuffers)
    {
        _cbResizeBuffers(this, true);
    }

    destroyRenderTarget();

    const auto result = _hkResizeBuffers->original<decltype(&resizeBuffers)>()(
        swapChain,
        bufferCount,
        width,
        height,
        newFormat,
        swapChainFlags
    );
    if (FAILED(result))
    {
        LOG_ERR("Original ResizeBuffers failed");
        return result;
    }

    ID3D11Device* dev = nullptr;
    if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&dev))))
    {
        LOG_ERR("ResizeBuffers: GetDevice failed");
        return result;
    }

    if (_pDevice != dev)
    {
        safeRelease(_pContext);
        safeRelease(_pDevice);
        _pDevice = dev;
        _pDevice->GetImmediateContext(&_pContext);
    }
    else
    {
        dev->Release();
    }

    if (!createRenderTarget(swapChain))
    {
        return result;
    }

    if (_cbResizeBuffers)
    {
        _cbResizeBuffers(this, false);
    }

    return result;
}

HRESULT memory::hooks::D3D11::present(IDXGISwapChain* swapChain, const UINT syncInterval, const UINT flags)
{
    return _instance->internalPresent(swapChain, syncInterval, flags);
}

HRESULT memory::hooks::D3D11::resizeBuffers(
    IDXGISwapChain*   swapChain,
    const UINT        bufferCount,
    const UINT        width,
    const UINT        height,
    const DXGI_FORMAT newFormat,
    const UINT        swapChainFlags
)
{
    return _instance->internalResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}
