#include "ProcessWatcher.h"

#include <cwchar>
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <utility>

ProcessWatcher::ProcessWatcher(std::wstring processName, std::chrono::seconds interval)
    : processName_(std::move(processName)),
      interval_(interval),
      stopRequested_(false),
      running_(false) {}

ProcessWatcher::~ProcessWatcher() {
    Stop();
}

void ProcessWatcher::OnStart(Callback callback) {
    onStart_ = std::move(callback);
}

void ProcessWatcher::OnStop(Callback callback) {
    onStop_ = std::move(callback);
}

void ProcessWatcher::Start() {
    stopRequested_ = false;
    worker_ = std::thread(&ProcessWatcher::Loop, this);
}

void ProcessWatcher::Stop() {
    stopRequested_ = true;
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool ProcessWatcher::IsProcessRunning() const {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName_.c_str()) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return found;
}

void ProcessWatcher::Loop() {
    std::wcout << L"[info] starting process monitoring: " << processName_ << L"\n";
    while (!stopRequested_) {
        const bool alive = IsProcessRunning();
        if (alive && !running_) {
            running_ = true;
            std::wcout << L"[info] process detected: " << processName_ << L"\n";
            if (onStart_) {
                onStart_();
            }
        }

        if (!alive && running_) {
            running_ = false;
            std::wcout << L"[info] process terminated: " << processName_ << L"\n";
            if (onStop_) {
                onStop_();
            }
        }

        std::this_thread::sleep_for(interval_);
    }
}
