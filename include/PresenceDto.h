#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct AssetsDto {
    std::string largeImage;
    std::string largeText;
    std::string smallImage;
    std::string smallText;
};

struct TimestampsDto {
    int64_t start = 0;
    int64_t end = 0;
};

struct PartyDto {
    std::string id;
    std::vector<int> size;
};

struct ButtonDto {
    std::string label;
    std::string url;
};

struct PresenceDto {
    std::string applicationId;
    int type = 0;
    std::string state;
    std::string details;
    bool hasAssets = false;
    bool hasTimestamps = false;
    bool hasParty = false;
    AssetsDto assets;
    TimestampsDto timestamps;
    PartyDto party;
    std::vector<ButtonDto> buttons;

    static PresenceDto FromJson(const nlohmann::json& value);
    std::string ToDiscordActivityJson() const;
};
