#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "SpotifyService.h"
#include "SetlistFmService.h"
#include "ConfigLoader.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "CallbackServer.h"
#include <shellapi.h>

// DirectX-Variablen
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward-Deklarationen
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward-Deklaration von ImGui_ImplWin32_WndProcHandler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Anwendungsstruktur für den UI-Zustand
struct AppState {
    // Services
    std::unique_ptr<SpotifyService> spotifyService;
    std::unique_ptr<SetlistFmService> setlistService;

    // UI-Zustandsvariablen
    char setlistIdInput[128] = "";
    std::string statusMessage = "Bereit";
    bool isLoading = false;
    bool hasSetlist = false;

    // Setlist-Daten
    SetlistFmService::Setlist currentSetlist;

    // Playlist-Erstellung
    bool createPlaylist = false;
    char playlistName[256] = "";
    bool playlistCreated = false;
    std::string playlistCreationStatus;
};

// Hauptfenster-UI-Funktion
void RenderMainUI(AppState& state);

// Service-Initialisierungsfunktion
bool InitializeServices(AppState& state);

int main(int, char**)
{
    // Fenster erstellen
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Setlist Spotify Generator", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Setlist Spotify Generator", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // DirectX initialisieren
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Keyboard-Navigation aktivieren
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Docking aktivieren
    ImGui::StyleColorsDark();  // Dunkles Farbschema

    // Platform/Renderer-Backends einrichten
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Anwendungszustand initialisieren
    AppState appState;
    if (!InitializeServices(appState)) {
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
        RenderMainUI(appState);

        // Rendern
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);  // VSync eingeschaltet
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Hauptfenster-UI
void RenderMainUI(AppState& state)
{
    // Hauptfenster erstellen
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Setlist Spotify Generator", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

    // Menüleiste
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Datei"))
        {
            if (ImGui::MenuItem("Beenden", "Alt+F4"))
            {
                // Anwendung beenden
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hilfe"))
        {
            if (ImGui::MenuItem("Über..."))
            {
                // Über-Dialog anzeigen
                ImGui::OpenPopup("Über Setlist Spotify Generator");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Über-Dialog
    if (ImGui::BeginPopupModal("Über Setlist Spotify Generator", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Setlist Spotify Generator v1.0");
        ImGui::Separator();
        ImGui::Text("Ein Tool zum Importieren von setlist.fm-Setlists in Spotify.");
        ImGui::Text("Entwickelt als Beispielprojekt für Bewerbungszwecke.");
        if (ImGui::Button("Schließen"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Statuszeile am oberen Rand
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.8f, 1.0f), "Status: %s", state.statusMessage.c_str());
    ImGui::Separator();

    // Setlist-ID-Eingabe
    ImGui::Text("Gib eine Setlist-ID von setlist.fm ein:");
    ImGui::InputText("##setlistId", state.setlistIdInput, IM_ARRAYSIZE(state.setlistIdInput));
    ImGui::SameLine();

    // Lade-Button
    bool loadDisabled = state.isLoading || strlen(state.setlistIdInput) == 0;
    if (loadDisabled) ImGui::BeginDisabled();
    if (ImGui::Button("Setlist laden") && !loadDisabled)
    {
        // Setlist laden
        state.isLoading = true;
        state.statusMessage = "Lade Setlist...";
        state.hasSetlist = false;

        // Setlist-ID abrufen
        std::string setlistId = state.setlistIdInput;

        // In einem separaten Thread laden, um UI nicht zu blockieren
        std::thread([&state, setlistId]() {
            auto setlist = state.setlistService->getSetlist(setlistId);
            if (setlist) {
                state.currentSetlist = *setlist;
                state.hasSetlist = true;
                state.statusMessage = "Setlist geladen: " + state.currentSetlist.artist + " @ " +
                    state.currentSetlist.venue;

                // Standardname für Playlist vorschlagen
                snprintf(state.playlistName, IM_ARRAYSIZE(state.playlistName),
                    "%s @ %s (%s)",
                    state.currentSetlist.artist.c_str(),
                    state.currentSetlist.venue.c_str(),
                    state.currentSetlist.eventDate.c_str());
            }
            else {
                state.statusMessage = "Fehler beim Laden der Setlist";
            }
            state.isLoading = false;
            }).detach();
    }
    if (loadDisabled) ImGui::EndDisabled();

    // Hilfstext
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
        "Hinweis: Die Setlist-ID findest du in der URL der Setlist auf setlist.fm");

    ImGui::Separator();

    // Unterer Bereich: Setlist-Anzeige und Playlist-Erstellung
    if (state.hasSetlist)
    {
        // Setlist-Informationen
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
            "Setlist: %s @ %s, %s, %s",
            state.currentSetlist.artist.c_str(),
            state.currentSetlist.venue.c_str(),
            state.currentSetlist.city.c_str(),
            state.currentSetlist.country.c_str());
        ImGui::Text("Datum: %s", state.currentSetlist.eventDate.c_str());
        ImGui::Text("Anzahl Songs: %zu", state.currentSetlist.songs.size());

        // Zwei Spalten: Songs und Playlist-Erstellung
        ImGui::Columns(2);

        // Songs-Liste
        ImGui::Text("Songs in der Setlist:");
        ImGui::BeginChild("SongsList", ImVec2(0, ImGui::GetContentRegionAvail().y), true);
        for (size_t i = 0; i < state.currentSetlist.songs.size(); i++)
        {
            const auto& song = state.currentSetlist.songs[i];
            std::string songDisplay = std::to_string(i + 1) + ". " + song.name;
            if (song.isCover && !song.coverArtist.empty()) {
                songDisplay += " (Cover von " + song.coverArtist + ")";
            }
            ImGui::TextWrapped("%s", songDisplay.c_str());
        }
        ImGui::EndChild();

        ImGui::NextColumn();

        // Playlist-Erstellung
        ImGui::Text("Spotify-Playlist erstellen:");
        ImGui::InputText("Playlist-Name", state.playlistName, IM_ARRAYSIZE(state.playlistName));

        // Playlist-Erstellungs-Button
        if (ImGui::Button("Playlist in Spotify erstellen", ImVec2(-1, 0)))
        {
            // Playlist erstellen
            state.createPlaylist = true;
            state.playlistCreated = false;
            state.playlistCreationStatus = "Erstelle Playlist...";

            // Songs in das gewünschte Format umwandeln
            std::vector<std::pair<std::string, std::string>> songList;
            for (const auto& song : state.currentSetlist.songs) {
                // Für Covers den Original-Künstler verwenden
                std::string artistToUse = song.isCover ? song.coverArtist : "";
                songList.push_back({ song.name, artistToUse });
            }

            // In einem separaten Thread importieren
            std::thread([&state, songList]() {
                bool success = state.spotifyService->importSetlistToSpotify(
                    state.playlistName,
                    state.currentSetlist.artist,
                    songList
                );

                if (success) {
                    state.playlistCreationStatus = "Playlist erfolgreich erstellt!";
                }
                else {
                    state.playlistCreationStatus = "Fehler beim Erstellen der Playlist";
                }

                state.playlistCreated = true;
                state.createPlaylist = false;
                }).detach();
        }

        // Status der Playlist-Erstellung
        if (state.createPlaylist) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Status: %s", state.playlistCreationStatus.c_str());

            // Fortschrittsbalken (könnte in Zukunft mit echtem Fortschritt aktualisiert werden)
            float progress = -1.0f;  // Unbestimmter Fortschrittsbalken
            ImGui::ProgressBar(progress, ImVec2(-1, 0));
        }
        else if (state.playlistCreated) {
            ImGui::TextColored(
                state.playlistCreationStatus.find("erfolgreich") != std::string::npos ?
                ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                "Status: %s", state.playlistCreationStatus.c_str()
            );
        }

        ImGui::Columns(1);
    }
    else if (state.isLoading) {
        // Lade-Animation
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
        ImGui::ProgressBar(-1.0f, ImVec2(200, 0));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Lade Setlist...").x) * 0.5f);
        ImGui::Text("Lade Setlist...");
    }
    else {
        // Anweisungs-Text
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Gib eine Setlist-ID ein, um zu beginnen").x) * 0.5f);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.5f);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Gib eine Setlist-ID ein, um zu beginnen");
    }

    ImGui::End();
}

// Service-Initialisierung
bool InitializeServices(AppState& state)
{
    try {
        // Konfiguration laden
        auto config = ConfigLoader::loadConfig("accessData.json");

        // SpotifyService initialisieren
        SpotifyService::AuthConfig spotifyConfig{
            config.spotify.client_id,
            config.spotify.client_secret,
            config.spotify.redirect_uri
        };

        state.spotifyService = std::make_unique<SpotifyService>(spotifyConfig);

        // SetlistFmService initialisieren
        SetlistFmService::Config setlistConfig{
            config.setlistfm.api_key
        };

        state.setlistService = std::make_unique<SetlistFmService>(setlistConfig);

        // Token laden oder Auth-Flow starten
        if (!state.spotifyService->loadTokenFromFile()) {
            state.statusMessage = "Bitte authentifiziere dich bei Spotify im Browser";

            // Browser öffnen
            std::string authUrl =
                "https://accounts.spotify.com/authorize?"
                "client_id=" + config.spotify.client_id +
                "&response_type=code"
                "&redirect_uri=" + config.spotify.redirect_uri +
                "&scope=user-read-private%20playlist-modify-public";

            ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);

            // Callback-Server in separatem Thread starten
            std::thread([&state]() {
                net::io_context ioc;
                CallbackServer server(ioc, 8080);

                server.start([&state](const std::string& code) {
                    state.statusMessage = "Auth-Code erhalten. Fordere Access-Token an...";
                    if (state.spotifyService->requestAccessToken(code)) {
                        state.statusMessage = "Authentifizierung erfolgreich!";
                    }
                    else {
                        state.statusMessage = "Fehler bei der Authentifizierung";
                    }
                    });

                ioc.run();
                }).detach();
        }
        else {
            state.statusMessage = "Bereit";
        }

        return true;
    }
    catch (const std::exception& e) {
        state.statusMessage = std::string("Fehler: ") + e.what();
        return false;
    }
}

// DirectX-Setup-Implementierungen
bool CreateDeviceD3D(HWND hWnd)
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
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
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
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
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