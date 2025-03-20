#include "ConfigLoader.h"
#include <string>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

ConfigLoader::AppConfig ConfigLoader::loadConfig(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + filename);
        }
        nlohmann::json j;
        file >> j;

        AppConfig config;

        // Prüfen auf altes Format und ggf. konvertieren
        if (j.contains("client_id") && !j.contains("spotify")) {
            // Altes Format
            config.spotify.client_id = j["client_id"].get<std::string>();
            config.spotify.client_secret = j["client_secret"].get<std::string>();
            config.spotify.redirect_uri = j.value("redirect_uri", "http://localhost:8080");

            // Setlistfm-Konfiguration wird leer sein
            config.setlistfm.api_key = "";
        }
        else {
            // Neues Format
            if (j.contains("spotify")) {
                config.spotify.client_id = j["spotify"]["client_id"].get<std::string>();
                config.spotify.client_secret = j["spotify"]["client_secret"].get<std::string>();
                config.spotify.redirect_uri = j["spotify"].value("redirect_uri", "http://localhost:8080");
            }

            if (j.contains("setlistfm")) {
                config.setlistfm.api_key = j["setlistfm"]["api_key"].get<std::string>();
            }
        }

        return config;
    }
    catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Error parsing config file: " + std::string(e.what()));
    }
}

void ConfigLoader::createDefaultConfig(const std::string& filename) {
    nlohmann::json j;
    j["spotify"]["client_id"] = "YOUR_CLIENT_ID";
    j["spotify"]["client_secret"] = "YOUR_CLIENT_SECRET";
    j["spotify"]["redirect_uri"] = "http://localhost:8080";
    j["setlistfm"]["api_key"] = "YOUR_SETLIST_FM_API_KEY";

    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not create config file: " + filename);
    }
    file << j.dump(4);
}