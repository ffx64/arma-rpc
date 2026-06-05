#include "AppInfo.h"
#include "HttpServer.h"
#include "PresenceController.h"
#include "ProcessWatcher.h"
#include "UpdateChecker.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace app {
std::string About() {
    return std::string(Name) + "\nVersion: " + Version + "\nAuthor: " + Author + "\nLicense: " + License;
}
}

namespace {
std::atomic<bool> g_stop{ false };

void SignalHandler(int) {
    g_stop = true;
}

bool HasArg(int argc, wchar_t* argv[], const std::wstring& expected) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == expected) {
            return true;
        }
    }
    return false;
}

uint16_t ReadPort(int argc, wchar_t* argv[], uint16_t fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == std::wstring(L"-port")) {
            const int parsed = _wtoi(argv[i + 1]);
            if (parsed > 0 && parsed <= 65535) {
                return static_cast<uint16_t>(parsed);
            }
        }
    }
    return fallback;
}
}

int wmain(int argc, wchar_t* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    if (HasArg(argc, argv, L"-version")) {
        std::cout << app::Version << "\n";
        return 0;
    }
    if (HasArg(argc, argv, L"-author")) {
        std::cout << app::Author << "\n";
        return 0;
    }
    if (HasArg(argc, argv, L"-license")) {
        std::cout << app::License << "\n";
        return 0;
    }
    if (HasArg(argc, argv, L"-about")) {
        std::cout << app::About() << "\n";
        return 0;
    }

    const bool debugMode = HasArg(argc, argv, L"-debug");
    const bool serveMode = HasArg(argc, argv, L"-serve");
    const uint16_t port = ReadPort(argc, argv, 7453);
    const std::wstring processName = debugMode ? L"ArmaReforgerSteamDiag.exe" : L"ArmaReforgerSteam.exe";

    if (debugMode) {
        std::cout << "[info] debug mode activated\n";
    }
    if (port != 7453) {
        std::cout << "[info] port changed to " << port << "\n";
    }

    PresenceController controller;
    std::mutex serverMutex;
    std::unique_ptr<HttpServer> server;

    if (serveMode) {
        server = std::make_unique<HttpServer>(port, controller);
        if (!server->Start()) {
            std::cerr << "[error] failed to start HTTP server\n";
            return 1;
        }

        std::cout << "[info] standalone server mode activated\n";
        UpdateChecker::CheckAsync(app::Name, app::Version);

        while (!g_stop) {
            controller.TickTimeout();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        server->Stop();
        controller.CloseRpc();
        return 0;
    }

    ProcessWatcher watcher(processName, std::chrono::seconds(30));
    watcher.OnStart([&]() {
        std::lock_guard<std::mutex> lock(serverMutex);
        if (server && server->IsRunning()) {
            std::cout << "[info] HTTP server already running, ignoring duplicate start\n";
            return;
        }
        server = std::make_unique<HttpServer>(port, controller);
        if (server->Start()) {
            UpdateChecker::CheckAsync(app::Name, app::Version);
        } else {
            std::cerr << "[error] failed to start HTTP server\n";
        }
    });

    watcher.OnStop([&]() {
        std::lock_guard<std::mutex> lock(serverMutex);
        if (server) {
            server->Stop();
            server.reset();
        }
        controller.CloseRpc();
    });

    watcher.Start();

    while (!g_stop) {
        controller.TickTimeout();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    watcher.Stop();
    {
        std::lock_guard<std::mutex> lock(serverMutex);
        if (server) {
            server->Stop();
        }
    }
    controller.CloseRpc();
    return 0;
}
