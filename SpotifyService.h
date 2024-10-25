#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <string>
#include <optional>
#include <fstream>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
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

    // Token-Management
    bool requestAccessToken(const std::string& auth_code);
    bool refreshAccessToken();
    bool loadTokenFromFile(const std::string& filename = "spotify_token.json");
    bool saveTokenToFile(const std::string& filename = "spotify_token.json") const;

    // API-Zugriffe
    std::optional<json> getTrack(const std::string& track_id);
    std::optional<json> searchTrack(const std::string& query);

private:
    AuthConfig config_;
    TokenInfo token_info_;
    std::unique_ptr<net::io_context> ioc_;
    std::unique_ptr<ssl::context> ctx_;

    std::optional<json> makeApiRequest(const std::string& host,
        const std::string& target,
        http::verb method = http::verb::get);

    bool ensureValidToken();
    std::string createAuthHeader() const;
};