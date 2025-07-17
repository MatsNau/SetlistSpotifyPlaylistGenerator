#pragma once
#include <string>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

class ConfigLoader {
public:
    struct SpotifyConfig {
        std::string client_id;
        std::string client_secret;
        std::string redirect_uri;
    };

    struct SetlistFmConfig {
        std::string api_key;
    };

    struct AppConfig {
        SpotifyConfig spotify;
        SetlistFmConfig setlistfm;
    };

    static AppConfig loadConfig(const std::string& filename);
    static void createDefaultConfig(const std::string& filename);
};