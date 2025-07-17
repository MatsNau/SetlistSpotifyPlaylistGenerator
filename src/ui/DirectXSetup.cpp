#include "DirectXSetup.h"

bool DirectXSetup::CreateDeviceD3D(HWND hWnd, ID3D11Device** device, ID3D11DeviceContext** context, IDXGISwapChain** swapChain, ID3D11RenderTargetView** renderTargetView)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, swapChain, device, &featureLevel, context) != S_OK)
        return false;

    CreateRenderTarget(*device, *swapChain, renderTargetView);
    return true;
}

void DirectXSetup::CleanupDeviceD3D(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain, ID3D11RenderTargetView* renderTargetView)
{
    CleanupRenderTarget(renderTargetView);
    if (swapChain) { swapChain->Release(); swapChain = NULL; }
    if (context) { context->Release(); context = NULL; }
    if (device) { device->Release(); device = NULL; }
}

void DirectXSetup::CreateRenderTarget(ID3D11Device* device, IDXGISwapChain* swapChain, ID3D11RenderTargetView** renderTargetView)
{
    ID3D11Texture2D* pBackBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    device->CreateRenderTargetView(pBackBuffer, NULL, renderTargetView);
    pBackBuffer->Release();
}

void DirectXSetup::CleanupRenderTarget(ID3D11RenderTargetView* renderTargetView)
{
    if (renderTargetView) { renderTargetView->Release(); renderTargetView = NULL; }
}
