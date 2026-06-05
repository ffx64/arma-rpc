#include "PresenceDto.h"

#include <stdexcept>
#include <utility>

namespace {
std::string StringField(const nlohmann::json& object, const std::string& name) {
    const auto it = object.find(name);
    return it != object.end() && it->is_string() ? it->get<std::string>() : "";
}

int IntField(const nlohmann::json& object, const std::string& name) {
    const auto it = object.find(name);
    return it != object.end() && it->is_number_integer() ? it->get<int>() : 0;
}

int64_t Int64Field(const nlohmann::json& object, const std::string& name) {
    const auto it = object.find(name);
    return it != object.end() && it->is_number_integer() ? it->get<int64_t>() : 0;
}
}

PresenceDto PresenceDto::FromJson(const nlohmann::json& value) {
    if (!value.is_object()) {
        throw std::runtime_error("payload must be an object");
    }

    PresenceDto dto;
    dto.applicationId = StringField(value, "application_id");
    dto.type = IntField(value, "type");
    dto.state = StringField(value, "state");
    dto.details = StringField(value, "details");

    if (const auto it = value.find("assets"); it != value.end() && it->is_object()) {
        const auto& assets = *it;
        dto.hasAssets = true;
        dto.assets.largeImage = StringField(assets, "large_image");
        dto.assets.largeText = StringField(assets, "large_text");
        dto.assets.smallImage = StringField(assets, "small_image");
        dto.assets.smallText = StringField(assets, "small_text");
    }

    if (const auto it = value.find("timestamps"); it != value.end() && it->is_object()) {
        const auto& timestamps = *it;
        dto.hasTimestamps = true;
        dto.timestamps.start = Int64Field(timestamps, "start");
        dto.timestamps.end = Int64Field(timestamps, "end");
    }

    if (const auto it = value.find("party"); it != value.end() && it->is_object()) {
        const auto& party = *it;
        dto.hasParty = true;
        dto.party.id = StringField(party, "id");
        if (const auto sizeIt = party.find("size"); sizeIt != party.end() && sizeIt->is_array()) {
            for (const auto& item : *sizeIt) {
                if (item.is_number_integer()) {
                    dto.party.size.push_back(item.get<int>());
                }
            }
        }
    }

    if (const auto it = value.find("buttons"); it != value.end() && it->is_array()) {
        for (const auto& item : *it) {
            if (!item.is_object()) {
                continue;
            }
            ButtonDto button;
            button.label = StringField(item, "label");
            button.url = StringField(item, "url");
            dto.buttons.push_back(std::move(button));
        }
    }

    if (dto.applicationId.empty()) {
        throw std::runtime_error("application_id is required");
    }

    return dto;
}

std::string PresenceDto::ToDiscordActivityJson() const {
    // Campos vazios precisam ser omitidos: o Discord valida os campos presentes
    // (ex.: state/details exigem ao menos 2 caracteres) e rejeita a activity inteira.
    nlohmann::json activity;
    activity["type"] = type;
    if (!state.empty()) {
        activity["state"] = state;
    }
    if (!details.empty()) {
        activity["details"] = details;
    }

    if (hasAssets) {
        nlohmann::json assetsJson = nlohmann::json::object();
        if (!assets.largeImage.empty()) {
            assetsJson["large_image"] = assets.largeImage;
        }
        if (!assets.largeText.empty()) {
            assetsJson["large_text"] = assets.largeText;
        }
        if (!assets.smallImage.empty()) {
            assetsJson["small_image"] = assets.smallImage;
        }
        if (!assets.smallText.empty()) {
            assetsJson["small_text"] = assets.smallText;
        }
        if (!assetsJson.empty()) {
            activity["assets"] = assetsJson;
        }
    }

    if (hasTimestamps) {
        nlohmann::json timestampsJson = nlohmann::json::object();
        if (timestamps.start > 0) {
            timestampsJson["start"] = timestamps.start;
        }
        if (timestamps.end > 0) {
            timestampsJson["end"] = timestamps.end;
        }
        if (!timestampsJson.empty()) {
            activity["timestamps"] = timestampsJson;
        }
    }

    if (hasParty) {
        nlohmann::json partyJson = nlohmann::json::object();
        if (!party.id.empty()) {
            partyJson["id"] = party.id;
        }
        if (!party.size.empty()) {
            partyJson["size"] = party.size;
        }
        if (!partyJson.empty()) {
            activity["party"] = partyJson;
        }
    }

    if (!buttons.empty()) {
        nlohmann::json buttonsJson = nlohmann::json::array();
        for (const ButtonDto& button : buttons) {
            if (button.label.empty() || button.url.empty()) {
                continue;
            }
            if (buttonsJson.size() == 2) {
                break; // o Discord aceita no máximo 2 botões
            }
            buttonsJson.push_back({
                { "label", button.label },
                { "url", button.url }
            });
        }
        if (!buttonsJson.empty()) {
            activity["buttons"] = buttonsJson;
        }
    }

    return activity.dump();
}
