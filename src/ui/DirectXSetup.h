#pragma once
#include <d3d11.h>
#include <windows.h>

/// <summary>
/// DirectX-Initialisierungsmethoden.
/// </summary>
class DirectXSetup {
public:
    static bool CreateDeviceD3D(HWND hWnd, ID3D11Device** device, ID3D11DeviceContext** context,
        IDXGISwapChain** swapChain, ID3D11RenderTargetView** renderTargetView);
    static void CleanupDeviceD3D(ID3D11Device* device, ID3D11DeviceContext* context,
        IDXGISwapChain* swapChain, ID3D11RenderTargetView* renderTargetView);
    static void CreateRenderTarget(ID3D11Device* device, IDXGISwapChain* swapChain,
        ID3D11RenderTargetView** renderTargetView);
    static void CleanupRenderTarget(ID3D11RenderTargetView* renderTargetView);
};