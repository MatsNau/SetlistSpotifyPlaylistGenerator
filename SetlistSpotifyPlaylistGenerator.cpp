// SetlistSpotifyPlaylistGenerator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "CallbackServer.h"
#include "SpotifyService.h"
#include "ConfigLoader.h"
#include <iostream>
#include <filesystem>
// Windows-spezifische Header
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
// Link mit der Windows Shell-Bibliothek
#pragma comment(lib, "shell32.lib")
int main() {
    // Überprüfen, ob die Konfigurationsdatei existiert
    /*/if (!std::filesystem::exists("AccessData.json")) {
        std::cout << "Creating default config file 'accessData.json'..." << std::endl;
        ConfigLoader::createDefaultConfig("accessData.json");
        std::cout << "Please edit 'AccessData.json' with your Spotify API credentials." << std::endl;
        return 0;
    }*/
    try {
        // Konfiguration laden
        auto config = ConfigLoader::loadConfig("accessData.json");


        // SpotifyService mit geladener Konfiguration initialisieren
        SpotifyService::AuthConfig authConfig{
            config.client_id,
            config.client_secret,
            config.redirect_uri
        };

        SpotifyService spotify(authConfig);

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
        if (auto result = spotify.searchTrack("Concerning Hobbits")) {
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
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

}
