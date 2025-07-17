// SetlistFmService.cpp
#include "SetlistFmService.h"
#include <iostream>
#include <curl/curl.h>

// Callback-Funktion für cURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    }
    catch (std::bad_alloc& e) {
        return 0;
    }
}

SetlistFmService::SetlistFmService(const Config& config)
    : config_(config) {
    // Initialisiere cURL global (nur einmal pro Anwendung)
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

SetlistFmService::~SetlistFmService() {
    // Cleanup cURL
    curl_global_cleanup();
}

std::optional<SetlistFmService::Setlist> SetlistFmService::getSetlist(const std::string& setlistId) {
    auto result = makeApiRequest("/rest/1.0/setlist/" + setlistId);
    if (result) {
        return parseSetlistJson(*result);
    }
    return std::nullopt;
}

std::optional<json> SetlistFmService::makeApiRequest(const std::string& target) {
    CURL* curl = curl_easy_init();
    std::string response_string;

    if (curl) {
        std::string url = "https://api.setlist.fm" + target;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Headers setzen
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        std::string api_key_header = "x-api-key: " + config_.api_key;
        headers = curl_slist_append(headers, api_key_header.c_str());
        headers = curl_slist_append(headers, "User-Agent: SetlistSpotifyGenerator/1.0");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Debug-Ausgabe
        std::cout << "Sende Anfrage an: " << url << std::endl;

        // Verbindung herstellen und Daten empfangen
        CURLcode res = curl_easy_perform(curl);

        // HTTP-Status abrufen
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            std::cout << "HTTP-Status: " << http_code << std::endl;

            if (http_code == 200) {
                try {
                    return json::parse(response_string);
                }
                catch (const json::parse_error& e) {
                    std::cerr << "JSON parse error: " << e.what() << std::endl;
                    std::cerr << "Response: " << response_string.substr(0, 200) << "..." << std::endl;
                }
            }
            else {
                std::cerr << "API error, HTTP-Status: " << http_code << std::endl;
                std::cerr << "Response: " << response_string << std::endl;
            }
        }
        else {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }
    }
    else {
        std::cerr << "Konnte cURL nicht initialisieren" << std::endl;
    }

    return std::nullopt;
}

SetlistFmService::Setlist SetlistFmService::parseSetlistJson(const json& j) {
    Setlist setlist;

    // Setlist-Metadaten extrahieren
    setlist.id = j["id"].get<std::string>();
    setlist.eventDate = j["eventDate"].get<std::string>();
    setlist.artist = j["artist"]["name"].get<std::string>();

    // Venue, City, Country Information
    if (j.contains("venue")) {
        setlist.venue = j["venue"]["name"].get<std::string>();

        if (j["venue"].contains("city")) {
            setlist.city = j["venue"]["city"]["name"].get<std::string>();
            setlist.country = j["venue"]["city"]["country"]["name"].get<std::string>();
        }
    }

    // Songs extrahieren
    if (j.contains("sets") && j["sets"].contains("set")) {
        for (const auto& set : j["sets"]["set"]) {
            if (set.contains("song")) {
                for (const auto& songJson : set["song"]) {
                    Song song;
                    song.name = songJson["name"].get<std::string>();
                    song.artist = setlist.artist; // Standard: Hauptkünstler

                    // Prüfen ob Cover
                    if (songJson.contains("cover")) {
                        song.isCover = true;
                        if (songJson["cover"].contains("name")) {
                            song.coverArtist = songJson["cover"]["name"].get<std::string>();
                        }
                    }

                    setlist.songs.push_back(song);
                }
            }
        }
    }

    return setlist;
}