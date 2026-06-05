#pragma once

#include <mutex>
#include <string>
#include <windows.h>

class DiscordRpcClient {
public:
    DiscordRpcClient();
    ~DiscordRpcClient();

    DiscordRpcClient(const DiscordRpcClient&) = delete;
    DiscordRpcClient& operator=(const DiscordRpcClient&) = delete;

    bool Connect(const std::string& applicationId);
    bool SetActivity(const std::string& activityJson);
    void Close();
    bool IsConnected() const;
    std::string LastError() const;

private:
    bool SendFrame(uint32_t opcode, const std::string& payload);
    bool ReadExact(void* buffer, size_t size);
    bool ReadFrame(uint32_t& opcode, std::string& payload);
    bool AwaitReady();
    bool AwaitCommandResponse(const std::string& nonce);
    void CloseLocked();

    mutable std::mutex mutex_;
    HANDLE pipe_;
    std::string applicationId_;
    std::string lastError_;
};
