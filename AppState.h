#pragma once
#include <string>
#include "SetlistFmService.h"
#include "SpotifyService.h"
/// <summary>
/// Beinhaltet den Zustand der Anwendung, der von der UI verwendet wird und die Services für Spotify und Setlist.fm.
/// </summary>
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