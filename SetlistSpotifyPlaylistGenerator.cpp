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
#include "SetlistFmService.h"
// Link mit der Windows Shell-Bibliothek
#pragma comment(lib, "shell32.lib")
int main() {
    try {
        // Konfiguration laden
        auto config = ConfigLoader::loadConfig("accessData.json");

        // SpotifyService initialisieren
        SpotifyService::AuthConfig spotifyConfig{
            config.spotify.client_id,
            config.spotify.client_secret,
            config.spotify.redirect_uri
        };
        SpotifyService spotify(spotifyConfig);

        // Token laden oder Auth-Flow starten
        if (!spotify.loadTokenFromFile()) {
            // Auth-Flow wie gehabt...
            std::string authUrl =
                "https://accounts.spotify.com/authorize?"
                "client_id=" + config.spotify.client_id +
                "&response_type=code"
                "&redirect_uri=" + config.spotify.redirect_uri +
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

        // SetlistFmService initialisieren
        SetlistFmService::Config setlistConfig{
            config.setlistfm.api_key
        };
        SetlistFmService setlistFm(setlistConfig);

        // Setlist-ID vom Benutzer abfragen
        std::string setlistId;
        std::cout << "Bitte Setlist-ID eingeben: ";
        std::getline(std::cin, setlistId);

        // Setlist abrufen
        std::cout << "\nSetlist wird abgerufen...\n" << std::endl;
        auto setlist = setlistFm.getSetlist(setlistId);

        if (setlist) {
            // Setlist-Informationen anzeigen
            std::cout << "Setlist: " << setlist->artist << " @ "
                << setlist->venue << ", " << setlist->city
                << ", " << setlist->country << std::endl;
            std::cout << "Datum: " << setlist->eventDate << std::endl;
            std::cout << "\nSongs:\n" << std::endl;

            // Songs auflisten
            int i = 1;
            for (const auto& song : setlist->songs) {
                std::cout << std::setw(3) << i << ". " << song.name;
                if (song.isCover && !song.coverArtist.empty()) {
                    std::cout << " (Cover von " << song.coverArtist << ")";
                }
                std::cout << std::endl;
                i++;
            }

            // Weitere Aktionen...
            std::cout << "\nMöchtest du diese Setlist als Spotify-Playlist erstellen? (j/n): ";
            std::string answer;
            std::getline(std::cin, answer);

            if (answer == "j" || answer == "J") {
                // Playlist-Name generieren
                std::string playlistName = setlist->artist + " @ " + setlist->venue + " (" + setlist->eventDate + ")";

                // Songs in das Format umwandeln, das die importSetlistToSpotify-Methode erwartet
                std::vector<std::pair<std::string, std::string>> songList;
                for (const auto& song : setlist->songs) {
                    // Für Covers den Original-Künstler verwenden
                    std::string artistToUse = song.isCover ? song.coverArtist : "";
                    songList.push_back({ song.name, artistToUse });
                }

                // Import starten
                if (spotify.importSetlistToSpotify(playlistName, setlist->artist, songList)) {
                    std::cout << "Playlist wurde erfolgreich erstellt!" << std::endl;
                }
                else {
                    std::cout << "Es gab Probleme beim Erstellen der Playlist." << std::endl;
                }
            }
        }
        else {
            std::cout << "Fehler beim Abrufen der Setlist!" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
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
