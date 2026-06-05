#pragma once

#include <chrono>
#include <mutex>

class RateLimiter {
public:
    RateLimiter(double requestsPerSecond, double burst);
    bool Allow();

private:
    std::mutex mutex_;
    double capacity_;
    double tokens_;
    double refillRate_;
    std::chrono::steady_clock::time_point lastRefill_;
};
