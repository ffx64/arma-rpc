#include "DiscordRpcClient.h"

#include "Utils.h"

#include <array>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace {
constexpr uint32_t kOpHandshake = 0;
constexpr uint32_t kOpFrame = 1;
constexpr uint32_t kOpClose = 2;
constexpr uint32_t kOpPing = 3;
constexpr uint32_t kOpPong = 4;

constexpr uint32_t kMaxFramePayload = 1024 * 1024;
constexpr int kMaxFramesToScan = 16;

std::string FieldOrDefault(const nlohmann::json& object, const char* name, const std::string& fallback) {
    const auto it = object.find(name);
    return it != object.end() && it->is_string() ? it->get<std::string>() : fallback;
}
}

DiscordRpcClient::DiscordRpcClient() : pipe_(INVALID_HANDLE_VALUE) {}

DiscordRpcClient::~DiscordRpcClient() {
    Close();
}

bool DiscordRpcClient::Connect(const std::string& applicationId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pipe_ != INVALID_HANDLE_VALUE && applicationId_ == applicationId) {
        return true;
    }
    CloseLocked();

    for (int index = 0; index < 10; ++index) {
        std::wostringstream path;
        path << L"\\\\?\\pipe\\discord-ipc-" << index;
        pipe_ = CreateFileW(path.str().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe_ != INVALID_HANDLE_VALUE) {
            break;
        }
    }

    if (pipe_ == INVALID_HANDLE_VALUE) {
        lastError_ = "Discord IPC pipe not found (is Discord running?)";
        std::cerr << "[rpc] failed to connect to Discord IPC pipe\n";
        return false;
    }

    const std::string handshake = "{\"v\":1,\"client_id\":\"" + util::JsonEscape(applicationId) + "\"}";
    if (!SendFrame(kOpHandshake, handshake) || !AwaitReady()) {
        CloseLocked();
        return false;
    }

    applicationId_ = applicationId;
    std::cout << "[rpc] connection established\n";
    return true;
}

bool DiscordRpcClient::SetActivity(const std::string& activityJson) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pipe_ == INVALID_HANDLE_VALUE) {
        lastError_ = "not connected to Discord";
        return false;
    }

    const std::string nonce = util::CreateNonce();
    std::ostringstream payload;
    payload << "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":" << util::CurrentProcessId()
            << ",\"activity\":" << activityJson << "},\"nonce\":\"" << nonce << "\"}";
    if (!SendFrame(kOpFrame, payload.str())) {
        CloseLocked();
        return false;
    }
    return AwaitCommandResponse(nonce);
}

void DiscordRpcClient::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pipe_ != INVALID_HANDLE_VALUE) {
        const std::string payload = "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":" +
            std::to_string(util::CurrentProcessId()) + ",\"activity\":null},\"nonce\":\"" + util::CreateNonce() + "\"}";
        SendFrame(kOpFrame, payload);
        CloseLocked();
    }
}

bool DiscordRpcClient::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pipe_ != INVALID_HANDLE_VALUE;
}

std::string DiscordRpcClient::LastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastError_;
}

bool DiscordRpcClient::SendFrame(uint32_t opcode, const std::string& payload) {
    std::array<uint32_t, 2> header{ opcode, static_cast<uint32_t>(payload.size()) };
    DWORD written = 0;
    if (!WriteFile(pipe_, header.data(), static_cast<DWORD>(sizeof(uint32_t) * header.size()), &written, nullptr)) {
        lastError_ = "failed to write ipc header";
        std::cerr << "[rpc] " << lastError_ << "\n";
        return false;
    }
    if (!WriteFile(pipe_, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr)) {
        lastError_ = "failed to write ipc payload";
        std::cerr << "[rpc] " << lastError_ << "\n";
        return false;
    }
    return true;
}

bool DiscordRpcClient::ReadExact(void* buffer, size_t size) {
    char* out = static_cast<char*>(buffer);
    size_t total = 0;
    while (total < size) {
        DWORD read = 0;
        if (!ReadFile(pipe_, out + total, static_cast<DWORD>(size - total), &read, nullptr) || read == 0) {
            return false;
        }
        total += read;
    }
    return true;
}

bool DiscordRpcClient::ReadFrame(uint32_t& opcode, std::string& payload) {
    std::array<uint32_t, 2> header{};
    if (!ReadExact(header.data(), sizeof(uint32_t) * header.size())) {
        lastError_ = "failed to read ipc header";
        std::cerr << "[rpc] " << lastError_ << "\n";
        return false;
    }
    if (header[1] > kMaxFramePayload) {
        lastError_ = "ipc frame too large";
        std::cerr << "[rpc] " << lastError_ << "\n";
        return false;
    }

    payload.resize(header[1]);
    if (header[1] > 0 && !ReadExact(payload.data(), payload.size())) {
        lastError_ = "failed to read ipc payload";
        std::cerr << "[rpc] " << lastError_ << "\n";
        return false;
    }
    opcode = header[0];
    return true;
}

bool DiscordRpcClient::AwaitReady() {
    for (int attempt = 0; attempt < kMaxFramesToScan; ++attempt) {
        uint32_t opcode = 0;
        std::string payload;
        if (!ReadFrame(opcode, payload)) {
            return false;
        }

        if (opcode == kOpPing) {
            if (!SendFrame(kOpPong, payload)) {
                return false;
            }
            continue;
        }
        if (opcode == kOpClose) {
            lastError_ = "handshake rejected by Discord: " + payload;
            std::cerr << "[rpc] " << lastError_ << "\n";
            return false;
        }
        if (opcode != kOpFrame) {
            continue;
        }

        try {
            const auto message = nlohmann::json::parse(payload);
            if (FieldOrDefault(message, "evt", "") == "READY") {
                return true;
            }
        } catch (const std::exception&) {
            // frame inesperado; continua aguardando o READY
        }
    }

    lastError_ = "READY event not received from Discord";
    std::cerr << "[rpc] " << lastError_ << "\n";
    return false;
}

bool DiscordRpcClient::AwaitCommandResponse(const std::string& nonce) {
    for (int attempt = 0; attempt < kMaxFramesToScan; ++attempt) {
        uint32_t opcode = 0;
        std::string payload;
        if (!ReadFrame(opcode, payload)) {
            CloseLocked();
            return false;
        }

        if (opcode == kOpPing) {
            if (!SendFrame(kOpPong, payload)) {
                CloseLocked();
                return false;
            }
            continue;
        }
        if (opcode == kOpClose) {
            lastError_ = "connection closed by Discord: " + payload;
            std::cerr << "[rpc] " << lastError_ << "\n";
            CloseLocked();
            return false;
        }
        if (opcode != kOpFrame) {
            continue;
        }

        try {
            const auto message = nlohmann::json::parse(payload);
            if (FieldOrDefault(message, "nonce", "") != nonce) {
                continue; // evento não relacionado a este comando
            }

            if (FieldOrDefault(message, "evt", "") == "ERROR") {
                std::string description = "unknown error";
                if (const auto it = message.find("data"); it != message.end() && it->is_object()) {
                    description = FieldOrDefault(*it, "message", description);
                }
                lastError_ = "activity rejected by Discord: " + description;
                std::cerr << "[rpc] " << lastError_ << "\n";
                return false; // erro de validação; a conexão continua utilizável
            }
            return true;
        } catch (const std::exception&) {
            // frame inválido; continua aguardando a resposta do comando
        }
    }

    lastError_ = "no response received from Discord";
    std::cerr << "[rpc] " << lastError_ << "\n";
    CloseLocked();
    return false;
}

void DiscordRpcClient::CloseLocked() {
    if (pipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
    }
}
