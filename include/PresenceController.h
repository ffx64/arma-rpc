#pragma once

#include "DiscordRpcClient.h"

#include <chrono>
#include <mutex>
#include <string>

class PresenceController {
public:
    PresenceController();

    std::string HandleUpdate(const std::string& body, int& statusCode);
    std::string HandleClose(int& statusCode);
    void CloseRpc();
    void TickTimeout();

private:
    std::mutex mutex_;
    DiscordRpcClient rpc_;
    std::chrono::steady_clock::time_point lastUpdate_;
    bool hasActiveSession_;
};
