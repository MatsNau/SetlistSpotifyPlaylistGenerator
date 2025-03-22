// Windows-Plattform definieren
#ifdef _WIN64
#define _AMD64_      
#elif defined(_WIN32)
#define _X86_       
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <WinUser.h>

#include <imgui.h>
#include <thread>
#include "UIRenderer.h"
#include "AppState.h"  



/// <summary>
/// Rendert die Benutzeroberfläche der Anwendung.
/// </summary>
/// <param name="state"></param>
void UIRenderer::RenderMainUI(AppState& state) {
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