// SetlistSpotifyPlaylistGenerator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "CallbackServer.h"
#include "SpotifyService.h"
#include <iostream>
// Windows-spezifische Header
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
// Link mit der Windows Shell-Bibliothek
#pragma comment(lib, "shell32.lib")
int main() {
    SpotifyService::AuthConfig config{
        "CLIENT_ID",
        "CLIENT_SECRET",
        "http://localhost:8080"
    };

    SpotifyService spotify(config);

    // Versuche gespeicherten Token zu laden
    if (!spotify.loadTokenFromFile()) {
        // Wenn kein Token vorhanden, starte Auth-Flow
        std::string authUrl =
            "https://accounts.spotify.com/authorize?"
            "client_id=" + config.client_id +
            "&response_type=code"
            "&redirect_uri=" + config.redirect_uri +
            "&scope=user-read-private%20playlist-modify-public";

        // Browser öffnen
        ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);

        // Callback-Server starten
        net::io_context ioc;
        CallbackServer server(ioc, 8080);

        server.start([&spotify](const std::string& code) {
            std::cout << "Received auth code. Requesting access token..." << std::endl;
            if (spotify.requestAccessToken(code)) {
                std::cout << "Successfully obtained access token!" << std::endl;
            }
            });

        ioc.run();
    }

    // Beispiel: Track suchen
    std::cout << "\nSuche nach Track..." << std::endl;
    if (auto result = spotify.searchTrack("Never Gonna Give You Up")) {
        try {
            // Debug-Ausgabe der gesamten JSON-Antwort
            std::cout << "Erhaltene JSON-Antwort:\n" << result->dump(2) << std::endl;

            // Zugriff auf die Track-Informationen
            if ((*result).contains("tracks") &&
                (*result)["tracks"].contains("items") &&
                !(*result)["tracks"]["items"].empty()) {

                auto track = (*result)["tracks"]["items"][0];
                std::cout << "\nGefundener Track:" << std::endl;
                std::cout << "Name: " << track["name"] << std::endl;

                if (track.contains("artists") && !track["artists"].empty()) {
                    std::cout << "Künstler: " << track["artists"][0]["name"] << std::endl;
                }

                if (track.contains("album")) {
                    std::cout << "Album: " << track["album"]["name"] << std::endl;
                }

                if (track.contains("duration_ms")) {
                    int duration_sec = track["duration_ms"].get<int>() / 1000;
                    std::cout << "Dauer: " << duration_sec / 60 << ":"
                        << std::setfill('0') << std::setw(2) << duration_sec % 60
                        << std::endl;
                }

                if (track.contains("external_urls") &&
                    track["external_urls"].contains("spotify")) {
                    std::cout << "Spotify URL: " << track["external_urls"]["spotify"] << std::endl;
                }
            }
        }
        catch (const json::exception& e) {
            std::cerr << "Fehler beim Parsen der JSON-Antwort: " << e.what() << std::endl;
        }
    }
    else {
        std::cout << "Fehler bei der Track-Suche!" << std::endl;
    }

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
