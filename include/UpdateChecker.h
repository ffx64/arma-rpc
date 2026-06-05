#pragma once

#include <string>

class UpdateChecker {
public:
    static void CheckAsync(const std::string& appName, const std::string& currentVersion);
};
