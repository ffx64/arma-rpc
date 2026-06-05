#include "RateLimiter.h"

RateLimiter::RateLimiter(double requestsPerSecond, double burst)
    : capacity_(burst),
      tokens_(burst),
      refillRate_(requestsPerSecond),
      lastRefill_(std::chrono::steady_clock::now()) {}

bool RateLimiter::Allow() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = now - lastRefill_;
    lastRefill_ = now;

    tokens_ += elapsed.count() * refillRate_;
    if (tokens_ > capacity_) {
        tokens_ = capacity_;
    }

    if (tokens_ < 1.0) {
        return false;
    }

    tokens_ -= 1.0;
    return true;
}
