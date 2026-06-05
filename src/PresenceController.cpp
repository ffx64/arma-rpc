#include "PresenceController.h"

#include "PresenceDto.h"

#include <iostream>
#include <nlohmann/json.hpp>

PresenceController::PresenceController()
    : lastUpdate_(std::chrono::steady_clock::now()), hasActiveSession_(false) {}

std::string PresenceController::HandleUpdate(const std::string& body, int& statusCode) {
    try {
        PresenceDto dto = PresenceDto::FromJson(nlohmann::json::parse(body));
        std::lock_guard<std::mutex> lock(mutex_);

        if (!rpc_.Connect(dto.applicationId)) {
            statusCode = 500;
            return "failed to initialize RPC: " + rpc_.LastError();
        }

        const std::string activityJson = dto.ToDiscordActivityJson();
        bool updated = rpc_.SetActivity(activityJson);
        if (!updated && !rpc_.IsConnected()) {
            // o Discord derrubou o pipe (restart, por exemplo); reconecta e tenta de novo
            updated = rpc_.Connect(dto.applicationId) && rpc_.SetActivity(activityJson);
        }
        if (!updated) {
            statusCode = 500;
            return "failed to update presence: " + rpc_.LastError();
        }

        lastUpdate_ = std::chrono::steady_clock::now();
        hasActiveSession_ = true;
        statusCode = 200;
        std::cout << "[controller] update received, state: " << dto.state << "\n";
        return "update processed successfully";
    } catch (const std::exception& ex) {
        statusCode = 400;
        return std::string("invalid payload: ") + ex.what();
    }
}

std::string PresenceController::HandleClose(int& statusCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hasActiveSession_) {
        statusCode = 400;
        return "no active RPC session";
    }

    rpc_.Close();
    hasActiveSession_ = false;
    statusCode = 200;
    return "RPC session closed";
}

void PresenceController::CloseRpc() {
    std::lock_guard<std::mutex> lock(mutex_);
    rpc_.Close();
    hasActiveSession_ = false;
}

void PresenceController::TickTimeout() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hasActiveSession_) {
        return;
    }

    const auto elapsed = std::chrono::steady_clock::now() - lastUpdate_;
    if (elapsed >= std::chrono::seconds(65)) {
        std::cout << "[rpc] no update received, session automatically closed\n";
        rpc_.Close();
        hasActiveSession_ = false;
    }
}
