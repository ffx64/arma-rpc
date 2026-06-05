#pragma once

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4244 4267)
#include <crow.h>
#pragma warning(pop)

#include <cstdint>
#include <string>
#include <thread>

#include "PresenceController.h"
#include "RateLimiter.h"

class HttpServer {
public:
    HttpServer(uint16_t port, PresenceController& controller);
    ~HttpServer();

    bool Start();
    void Stop();
    bool IsRunning() const;

private:
    void ConfigureRoutes();
    crow::response TextResponse(int statusCode, const std::string& body) const;

    uint16_t port_;
    PresenceController& controller_;
    RateLimiter limiter_;
    std::atomic<bool> running_;
    crow::SimpleApp app_;
    std::thread serverThread_;
};
