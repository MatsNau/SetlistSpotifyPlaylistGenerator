#pragma once
#include <string>
#include <optional>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SpotifyService {
public:
    struct AuthConfig {
        std::string client_id;
        std::string client_secret;
        std::string redirect_uri;
    };

    struct TokenInfo {
        std::string access_token;
        std::string refresh_token;
        int64_t expires_in;
        std::string token_type;
        std::chrono::system_clock::time_point timestamp;
        bool isExpired() const;
    };

    SpotifyService(const AuthConfig& config);
    ~SpotifyService();

    // Token-Management
    bool requestAccessToken(const std::string& auth_code);
    bool refreshAccessToken();
    bool loadTokenFromFile(const std::string& filename = "spotify_token.json");
    bool saveTokenToFile(const std::string& filename = "spotify_token.json") const;

    // API-Zugriffe
    std::optional<json> getTrack(const std::string& track_id);
    std::optional<json> searchTrack(const std::string& query);
    std::optional<std::string> searchTrackId(const std::string& trackName, const std::string& artist);

    // Playlist-Management
    std::optional<std::string> createPlaylist(const std::string& name, const std::string& description = "");
    bool addTracksToPlaylist(const std::string& playlistId, const std::vector<std::string>& trackIds);
    bool importSetlistToSpotify(const std::string& playlistName,
        const std::string& artist,
        const std::vector<std::pair<std::string, std::string>>& songs);

private:
    AuthConfig config_;
    TokenInfo token_info_;

    std::optional<json> makeApiRequest(
        const std::string& endpoint,
        const std::string& method = "GET",
        const json& body = nullptr);

    bool ensureValidToken();
    std::string createAuthHeader() const;
    static std::string urlEncode(const std::string& value);
};