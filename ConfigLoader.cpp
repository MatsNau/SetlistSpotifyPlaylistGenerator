#include "ConfigLoader.h"
#include <string>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

ConfigLoader::SpotifyConfig ConfigLoader::loadConfig(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + filename);
        }

        nlohmann::json j;
        file >> j;

        SpotifyConfig config;
        config.client_id = j["client_id"].get<std::string>();
        config.client_secret = j["client_secret"].get<std::string>();
        config.redirect_uri = j.value("redirect_uri", "http://localhost:8080");

        return config;
    }
    catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Error parsing config file: " + std::string(e.what()));
    }
}

void ConfigLoader::createDefaultConfig(const std::string& filename) {
    nlohmann::json j;
    j["client_id"] = "YOUR_CLIENT_ID";
    j["client_secret"] = "YOUR_CLIENT_SECRET";
    j["redirect_uri"] = "http://localhost:8080";

    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not create config file: " + filename);
    }

    file << j.dump(4);
}