
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Windows-Plattform definieren
#ifdef _WIN64
#define _AMD64_
#elif defined(_WIN32)
#define _X86_
#endif

// Windows-Header
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// DirectX-Header
#include <d3d11.h>
#include <tchar.h>

// ImGui-Header
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

// Eigene Projekt-Header
#include "AppState.h"
#include "UIRenderer.h"
#include "AppInitializer.h"
#include "DirectXSetup.h"

// Forward-Deklaration von ImGui_ImplWin32_WndProcHandler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 Nachrichtenverarbeitung
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// DirectX-Variablen
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

int main(int, char**)
{
    // Fenster erstellen
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Setlist Spotify Generator", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Setlist Spotify Generator", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // DirectX initialisieren
    if (!DirectXSetup::CreateDeviceD3D(hwnd, &g_pd3dDevice, &g_pd3dDeviceContext, &g_pSwapChain, &g_mainRenderTargetView))
    {
        DirectXSetup::CleanupDeviceD3D(g_pd3dDevice, g_pd3dDeviceContext, g_pSwapChain, g_mainRenderTargetView);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Fenster anzeigen
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // ImGui einrichten
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // Platform/Renderer-Backends einrichten
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Anwendungszustand initialisieren
    AppState appState;
    if (!AppInitializer::InitializeServices(appState)) {
        MessageBoxA(hwnd, "Fehler bei der Initialisierung der Services", "Fehler", MB_ICONERROR);
    }

    // Hauptschleife
    bool done = false;
    while (!done)
    {
        // Windows-Nachrichten verarbeiten
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // ImGui Frame starten
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // UI rendern
        UIRenderer::RenderMainUI(appState);

        // Rendern
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DirectXSetup::CleanupDeviceD3D(g_pd3dDevice, g_pd3dDeviceContext, g_pSwapChain, g_mainRenderTargetView);
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Win32 Nachrichtenverarbeitung
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            DirectXSetup::CleanupRenderTarget(g_mainRenderTargetView);
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            DirectXSetup::CreateRenderTarget(g_pd3dDevice, g_pSwapChain, &g_mainRenderTargetView);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Deaktivieren des ALT-Anwendungsmenüs
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}