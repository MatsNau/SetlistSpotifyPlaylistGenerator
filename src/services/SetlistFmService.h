#pragma once
#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SetlistFmService {
public:
    struct Config {
        std::string api_key;
    };

    struct Song {
        std::string name;
        std::string artist;
        bool isCover = false;
        std::string coverArtist = "";
    };

    struct Setlist {
        std::string id;
        std::string eventDate;
        std::string artist;
        std::string venue;
        std::string city;
        std::string country;
        std::vector<Song> songs;
    };

    explicit SetlistFmService(const Config& config);
    ~SetlistFmService();

    // Hauptmethode: Setlist über ID abrufen
    std::optional<Setlist> getSetlist(const std::string& setlistId);

private:
    Config config_;

    std::optional<json> makeApiRequest(const std::string& target);
    Setlist parseSetlistJson(const json& j);
};