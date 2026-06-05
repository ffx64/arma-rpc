#include "Utils.h"

#include <chrono>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>
#include <windows.h>

namespace util {
std::wstring Widen(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string Narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::string JsonEscape(const std::string& value) {
    std::ostringstream out;
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
            } else {
                out << ch;
            }
            break;
        }
    }
    return out.str();
}

std::string CreateNonce() {
    static std::mt19937_64 rng{ std::random_device{}() };
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::ostringstream out;
    out << std::hex << now << rng();
    return out.str();
}

uint32_t CurrentProcessId() {
    return static_cast<uint32_t>(GetCurrentProcessId());
}

std::string ToLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}
}
