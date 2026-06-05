#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

class ProcessWatcher {
public:
    using Callback = std::function<void()>;

    ProcessWatcher(std::wstring processName, std::chrono::seconds interval);
    ~ProcessWatcher();

    void OnStart(Callback callback);
    void OnStop(Callback callback);
    void Start();
    void Stop();

private:
    bool IsProcessRunning() const;
    void Loop();

    std::wstring processName_;
    std::chrono::seconds interval_;
    Callback onStart_;
    Callback onStop_;
    std::atomic<bool> stopRequested_;
    std::atomic<bool> running_;
    std::thread worker_;
};
