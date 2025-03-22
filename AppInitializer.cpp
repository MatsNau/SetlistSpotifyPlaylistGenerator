#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "AppInitializer.h"
#include "ConfigLoader.h"
#include <thread>
#include <shellapi.h>
#include "CallbackServer.h"

bool AppInitializer::InitializeServices(AppState& state) {
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