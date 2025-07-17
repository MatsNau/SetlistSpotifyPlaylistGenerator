#include "SpotifyService.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
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

bool SpotifyService::TokenInfo::isExpired() const {
    auto now = std::chrono::system_clock::now();
    return (now - timestamp) >= std::chrono::seconds(expires_in - 300); // 5 Minuten Puffer
}

SpotifyService::SpotifyService(const AuthConfig& config)
    : config_(config) {
    // cURL global initialisieren
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

SpotifyService::~SpotifyService() {
    // cURL global bereinigen
    curl_global_cleanup();
}

bool SpotifyService::requestAccessToken(const std::string& auth_code) {
    // URL für Token-Anfrage
    std::string url = "https://accounts.spotify.com/api/token";

    // Request-Body
    std::string request_body =
        "grant_type=authorization_code"
        "&code=" + auth_code +
        "&redirect_uri=" + urlEncode(config_.redirect_uri) +
        "&client_id=" + config_.client_id +
        "&client_secret=" + config_.client_secret;

    CURL* curl = curl_easy_init();
    std::string response_string;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Headers setzen
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Anfrage senden
        CURLcode res = curl_easy_perform(curl);

        // HTTP-Status abrufen
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK && http_code == 200) {
            try {
                auto j = json::parse(response_string);
                token_info_.access_token = j["access_token"];
                token_info_.refresh_token = j["refresh_token"];
                token_info_.expires_in = j["expires_in"];
                token_info_.token_type = j["token_type"];
                token_info_.timestamp = std::chrono::system_clock::now();

                saveTokenToFile();
                return true;
            }
            catch (const json::parse_error& e) {
                std::cerr << "Token-Parsing-Fehler: " << e.what() << std::endl;
            }
        }
        else {
            std::cerr << "Token-Anfrage fehlgeschlagen: " << curl_easy_strerror(res) << std::endl;
            std::cerr << "HTTP-Status: " << http_code << ", Response: " << response_string << std::endl;
        }
    }

    return false;
}

bool SpotifyService::refreshAccessToken() {
    // URL für Token-Anfrage
    std::string url = "https://accounts.spotify.com/api/token";

    // Request-Body
    std::string request_body =
        "grant_type=refresh_token"
        "&refresh_token=" + token_info_.refresh_token +
        "&client_id=" + config_.client_id +
        "&client_secret=" + config_.client_secret;

    CURL* curl = curl_easy_init();
    std::string response_string;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Headers setzen
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Anfrage senden
        CURLcode res = curl_easy_perform(curl);

        // HTTP-Status abrufen
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK && http_code == 200) {
            try {
                auto j = json::parse(response_string);
                token_info_.access_token = j["access_token"];
                token_info_.expires_in = j["expires_in"];
                token_info_.token_type = j["token_type"];
                token_info_.timestamp = std::chrono::system_clock::now();

                saveTokenToFile();
                return true;
            }
            catch (const json::parse_error& e) {
                std::cerr << "Token-Refresh-Parsing-Fehler: " << e.what() << std::endl;
            }
        }
        else {
            std::cerr << "Token-Refresh fehlgeschlagen: " << curl_easy_strerror(res) << std::endl;
            std::cerr << "HTTP-Status: " << http_code << ", Response: " << response_string << std::endl;
        }
    }

    return false;
}

bool SpotifyService::loadTokenFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        json j;
        file >> j;

        token_info_.access_token = j["access_token"];
        token_info_.refresh_token = j["refresh_token"];
        token_info_.expires_in = j["expires_in"];
        token_info_.token_type = j["token_type"];
        token_info_.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(j["timestamp_ms"].get<int64_t>())
        );

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading token: " << e.what() << std::endl;
        return false;
    }
}

bool SpotifyService::saveTokenToFile(const std::string& filename) const {
    try {
        json j;
        j["access_token"] = token_info_.access_token;
        j["refresh_token"] = token_info_.refresh_token;
        j["expires_in"] = token_info_.expires_in;
        j["token_type"] = token_info_.token_type;
        j["timestamp_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            token_info_.timestamp.time_since_epoch()
        ).count();

        std::ofstream file(filename);
        file << j.dump(4);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving token: " << e.what() << std::endl;
        return false;
    }
}

std::optional<json> SpotifyService::getTrack(const std::string& track_id) {
    if (!ensureValidToken()) return std::nullopt;
    return makeApiRequest("/v1/tracks/" + track_id);
}

std::optional<json> SpotifyService::searchTrack(const std::string& query) {
    if (!ensureValidToken()) return std::nullopt;
    return makeApiRequest("/v1/search?q=" + urlEncode(query) + "&type=track&limit=1");
}

std::optional<std::string> SpotifyService::searchTrackId(const std::string& trackName, const std::string& artist) {
    if (!ensureValidToken()) return std::nullopt;

    std::string query = "track:" + trackName + " artist:" + artist;
    auto result = makeApiRequest("/v1/search?q=" + urlEncode(query) + "&type=track&limit=1");

    if (result) {
        try {
            if ((*result).contains("tracks") &&
                (*result)["tracks"].contains("items") &&
                !(*result)["tracks"]["items"].empty()) {

                auto track = (*result)["tracks"]["items"][0];
                return track["id"].get<std::string>();
            }
        }
        catch (const json::exception& e) {
            std::cerr << "Fehler beim Parsen der Track-Suche: " << e.what() << std::endl;
        }
    }

    return std::nullopt;
}

std::optional<std::string> SpotifyService::createPlaylist(const std::string& name, const std::string& description) {
    if (!ensureValidToken()) return std::nullopt;

    // Benutzerprofil abrufen, um User-ID zu erhalten
    auto userProfile = makeApiRequest("/v1/me");
    if (!userProfile) {
        std::cerr << "Konnte Benutzerprofil nicht abrufen." << std::endl;
        return std::nullopt;
    }

    std::string userId = (*userProfile)["id"].get<std::string>();

    // Playlist erstellen
    json body = {
        {"name", name},
        {"description", description},
        {"public", true}
    };

    auto result = makeApiRequest("/v1/users/" + userId + "/playlists", "POST", body);

    if (result) {
        return (*result)["id"].get<std::string>();
    }

    return std::nullopt;
}

bool SpotifyService::addTracksToPlaylist(const std::string& playlistId, const std::vector<std::string>& trackIds) {
    if (!ensureValidToken() || trackIds.empty()) return false;

    // Track-URIs erstellen
    std::vector<std::string> trackUris;
    for (const auto& id : trackIds) {
        trackUris.push_back("spotify:track:" + id);
    }

    // JSON-Body für Track-Hinzufügen
    json body = {
        {"uris", trackUris}
    };

    auto result = makeApiRequest("/v1/playlists/" + playlistId + "/tracks", "POST", body);

    return result.has_value();
}

bool SpotifyService::importSetlistToSpotify(const std::string& playlistName,
    const std::string& artist,
    const std::vector<std::pair<std::string, std::string>>& songs) {
    if (!ensureValidToken()) return false;

    // Playlist erstellen
    std::cout << "Erstelle Playlist '" << playlistName << "'..." << std::endl;
    auto playlistId = createPlaylist(playlistName, "Setlist von " + artist);
    if (!playlistId) {
        std::cerr << "Konnte Playlist nicht erstellen." << std::endl;
        return false;
    }

    // Tracks suchen
    std::cout << "Suche nach Songs..." << std::endl;
    std::vector<std::string> trackIds;
    int foundCount = 0;
    int totalCount = songs.size();

    for (const auto& [title, songArtist] : songs) {
        std::cout << "  Suche: " << title;

        // Verwende den Song-spezifischen Künstler, falls vorhanden, sonst den Hauptkünstler
        std::string artistToUse = songArtist.empty() ? artist : songArtist;
        std::cout << " (Künstler: " << artistToUse << ")... ";

        auto trackId = searchTrackId(title, artistToUse);
        if (trackId) {
            trackIds.push_back(*trackId);
            std::cout << "gefunden!" << std::endl;
            foundCount++;
        }
        else {
            std::cout << "nicht gefunden." << std::endl;
        }
    }

    // Gefundene Tracks zur Playlist hinzufügen
    if (!trackIds.empty()) {
        std::cout << "\nFüge " << trackIds.size() << " von " << totalCount
            << " Songs zur Playlist hinzu..." << std::endl;

        if (addTracksToPlaylist(*playlistId, trackIds)) {
            std::cout << "Playlist erfolgreich erstellt und Songs hinzugefügt!" << std::endl;
            std::cout << "Gefunden: " << foundCount << " von " << totalCount << " Songs." << std::endl;
            return true;
        }
        else {
            std::cerr << "Fehler beim Hinzufügen der Songs zur Playlist." << std::endl;
        }
    }
    else {
        std::cerr << "Keine Songs gefunden. Playlist ist leer." << std::endl;
    }

    return false;
}

std::optional<json> SpotifyService::makeApiRequest(
    const std::string& endpoint,
    const std::string& method,
    const json& body) {

    if (!ensureValidToken()) return std::nullopt;

    CURL* curl = curl_easy_init();
    std::string response_string;

    if (curl) {
        std::string url = "https://api.spotify.com" + endpoint;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // HTTP-Methode setzen
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }
        else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        }
        else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        // Headers setzen
        struct curl_slist* headers = NULL;
        std::string auth_header = "Authorization: " + createAuthHeader();
        headers = curl_slist_append(headers, auth_header.c_str());

        // Body setzen, falls vorhanden
        std::string body_str;
        if (body != nullptr) {
            body_str = body.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Anfrage senden
        CURLcode res = curl_easy_perform(curl);

        // HTTP-Status abrufen
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            if (http_code >= 200 && http_code < 300) {
                try {
                    if (!response_string.empty()) {
                        return json::parse(response_string);
                    }
                    else {
                        // Manche Endpunkte geben leere Antworten zurück
                        return json::object();
                    }
                }
                catch (const json::parse_error& e) {
                    std::cerr << "JSON-Parsing-Fehler: " << e.what() << std::endl;
                    std::cerr << "Response: " << response_string.substr(0, 200) << "..." << std::endl;
                }
            }
            else {
                std::cerr << "API-Fehler, HTTP-Status: " << http_code << std::endl;
                std::cerr << "Response: " << response_string << std::endl;
            }
        }
        else {
            std::cerr << "cURL-Fehler: " << curl_easy_strerror(res) << std::endl;
        }
    }
    else {
        std::cerr << "Konnte cURL nicht initialisieren" << std::endl;
    }

    return std::nullopt;
}

bool SpotifyService::ensureValidToken() {
    if (token_info_.isExpired()) {
        return refreshAccessToken();
    }
    return true;
}

std::string SpotifyService::createAuthHeader() const {
    return token_info_.token_type + " " + token_info_.access_token;
}

std::string SpotifyService::urlEncode(const std::string& value) {
    char* encoded = curl_easy_escape(nullptr, value.c_str(), static_cast<int>(value.length()));
    std::string result(encoded);
    curl_free(encoded);
    return result;
}