#include "SpotifyService.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

bool SpotifyService::TokenInfo::isExpired() const {
    auto now = std::chrono::system_clock::now();
    return (now - timestamp) >= std::chrono::seconds(expires_in - 300); // 5 Minuten Puffer
}

SpotifyService::SpotifyService(const AuthConfig& config)
    : config_(config) {
    ioc_ = std::make_unique<net::io_context>();
    ctx_ = std::make_unique<ssl::context>(ssl::context::tlsv12_client);
    ctx_->set_default_verify_paths();
}

bool SpotifyService::requestAccessToken(const std::string& auth_code) {
    std::string request_body =
        "grant_type=authorization_code"
        "&code=" + auth_code +
        "&redirect_uri=" + config_.redirect_uri +
        "&client_id=" + config_.client_id +
        "&client_secret=" + config_.client_secret;

    try {
        tcp::resolver resolver(*ioc_);
        beast::ssl_stream<beast::tcp_stream> stream(*ioc_, *ctx_);

        auto const results = resolver.resolve("accounts.spotify.com", "443");
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{ http::verb::post, "/api/token", 11 };
        req.set(http::field::host, "accounts.spotify.com");
        req.set(http::field::content_type, "application/x-www-form-urlencoded");
        req.body() = request_body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        if (res.result() == http::status::ok) {
            auto j = json::parse(res.body());
            token_info_.access_token = j["access_token"];
            token_info_.refresh_token = j["refresh_token"];
            token_info_.expires_in = j["expires_in"];
            token_info_.token_type = j["token_type"];
            token_info_.timestamp = std::chrono::system_clock::now();

            saveTokenToFile();
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error requesting access token: " << e.what() << std::endl;
    }
    return false;
}

bool SpotifyService::refreshAccessToken() {
    std::string request_body =
        "grant_type=refresh_token"
        "&refresh_token=" + token_info_.refresh_token +
        "&client_id=" + config_.client_id +
        "&client_secret=" + config_.client_secret;

    try {
        tcp::resolver resolver(*ioc_);
        beast::ssl_stream<beast::tcp_stream> stream(*ioc_, *ctx_);

        auto const results = resolver.resolve("accounts.spotify.com", "443");
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{ http::verb::post, "/api/token", 11 };
        req.set(http::field::host, "accounts.spotify.com");
        req.set(http::field::content_type, "application/x-www-form-urlencoded");
        req.body() = request_body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        if (res.result() == http::status::ok) {
            auto j = json::parse(res.body());
            token_info_.access_token = j["access_token"];
            token_info_.expires_in = j["expires_in"];
            token_info_.token_type = j["token_type"];
            token_info_.timestamp = std::chrono::system_clock::now();

            saveTokenToFile();
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error refreshing token: " << e.what() << std::endl;
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

    return makeApiRequest("api.spotify.com", "/v1/tracks/" + track_id);
}

std::optional<json> SpotifyService::searchTrack(const std::string& query) {
    if (!ensureValidToken()) return std::nullopt;

    std::string encoded_query;
    std::ostringstream hex_stream;

    for (char c : query) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded_query += c;
        }
        else if (c == ' ') {
            encoded_query += "%20";
        }
        else {
            hex_stream.str("");
            hex_stream.clear();
            hex_stream << std::hex << std::uppercase << std::setw(2)
                << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
            encoded_query += "%" + hex_stream.str();
        }
    }

    return makeApiRequest("api.spotify.com",
        "/v1/search?q=" + encoded_query + "&type=track&limit=1");
}

std::optional<json> SpotifyService::makeApiRequest(
    const std::string& host,
    const std::string& target,
    http::verb method) {
    try {
        tcp::resolver resolver(*ioc_);
        beast::ssl_stream<beast::tcp_stream> stream(*ioc_, *ctx_);

        auto const results = resolver.resolve(host, "443");
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{ method, target, 11 };
        req.set(http::field::host, host);
        req.set(http::field::authorization, createAuthHeader());
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        if (res.result() == http::status::ok) {
            return json::parse(res.body());
        }
    }
    catch (const std::exception& e) {
        std::cerr << "API request error: " << e.what() << std::endl;
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