#include "UpdateChecker.h"

#include "Utils.h"

#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>

namespace {
std::string GetLatestReleaseJson() {
    HINTERNET session = WinHttpOpen(
        L"reforger-rpc/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!session) {
        return "";
    }

    HINTERNET connect = WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connect) {
        WinHttpCloseHandle(session);
        return "";
    }

    HINTERNET request = WinHttpOpenRequest(
        connect,
        L"GET",
        L"/repos/ffx64/presence-reforger/releases/latest",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    const wchar_t* headers = L"User-Agent: reforger-rpc/1.0\r\n";
    std::string result;

    if (WinHttpSendRequest(
            request,
            headers,
            static_cast<DWORD>(-1L),
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0) &&
        WinHttpReceiveResponse(request, nullptr)) {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX);

        if (statusCode == 200) {
            DWORD available = 0;
            while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
                std::string chunk(available, '\0');
                DWORD read = 0;
                if (!WinHttpReadData(request, chunk.data(), available, &read)) {
                    break;
                }
                chunk.resize(read);
                result += chunk;
            }
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return result;
}
}

void UpdateChecker::CheckAsync(const std::string& appName, const std::string& currentVersion) {
    std::thread([appName, currentVersion]() {
        const std::string response = GetLatestReleaseJson();
        if (response.empty()) {
            return;
        }

        nlohmann::json release;
        try {
            release = nlohmann::json::parse(response);
        } catch (const nlohmann::json::exception&) {
            return;
        }

        const std::string tag = release.value("tag_name", "");
        const std::string url = release.value("html_url", "");
        if (tag.empty() || url.empty() || tag == currentVersion) {
            return;
        }

        const std::wstring title = util::Widen("Update available for " + appName);
        const std::wstring text = util::Widen("A new version is available.\nVersion: " + tag + "\nUpdate now?");
        const int result = MessageBoxW(nullptr, text.c_str(), title.c_str(), MB_OKCANCEL | MB_ICONINFORMATION);
        if (result == IDOK) {
            ShellExecuteW(nullptr, L"open", util::Widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }).detach();
}
