#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace util {
std::wstring Widen(const std::string& value);
std::string Narrow(const std::wstring& value);
std::string JsonEscape(const std::string& value);
std::string CreateNonce();
uint32_t CurrentProcessId();
std::string ToLower(std::string value);
}
