#include "app.h"
#include "utils/config.h"
#include "utils/connection_info.h"

#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace sqlbot {

#ifdef _WIN32
namespace {

constexpr int kInputId = 1001;
constexpr int kRunId = 1002;
constexpr int kOutputId = 1003;
constexpr int kInfoId = 1004;

struct GuiState {
    RuntimeConfig config;
    std::unique_ptr<std::future<std::string>> inflight;
    std::string last_error;
};

std::wstring ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring out(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), size);
    if (!out.empty()) {
        out.pop_back();
    }
    return out;
}

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), size, nullptr, nullptr);
    if (!out.empty()) {
        out.pop_back();
    }
    return out;
}

// Connection summary plus the latest error, formatted for a static control (CRLF breaks).
std::wstring BuildInfoText(const GuiState& state) {
    std::string text = DescribeConnection(state.config);
    text += "\nLast error: ";
    text += state.last_error.empty() ? std::string("none") : state.last_error;
    std::wstring out;
    out.reserve(text.size() + 16);
    for (const wchar_t ch : ToWide(text)) {
        if (ch == L'\n') {
            out += L"\r\n";
        }
        else {
            out += ch;
        }
    }
    return out;
}
void RefreshInfo(const HWND hwnd, const GuiState& state) {
    SetWindowTextW(GetDlgItem(hwnd, kInfoId), BuildInfoText(state).c_str());
}

LRESULT CALLBACK WndProc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam) {
    auto* state = reinterpret_cast<GuiState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* create = reinterpret_cast<CREATESTRUCT*>(lparam);
            state = reinterpret_cast<GuiState*>(create->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

            CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                          10, 10, 620, 24, hwnd, reinterpret_cast<HMENU>(kInputId), nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Run", WS_VISIBLE | WS_CHILD,
                          640, 10, 80, 24, hwnd, reinterpret_cast<HMENU>(kRunId), nullptr, nullptr);
            CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFT | SS_NOPREFIX,
                10, 45, 710, 130, hwnd, reinterpret_cast<HMENU>(kInfoId), nullptr, nullptr);
            CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                10, 185, 710, 370, hwnd, reinterpret_cast<HMENU>(kOutputId), nullptr, nullptr);
            if (state != nullptr) {
                RefreshInfo(hwnd, *state);
            }
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wparam) == kRunId && state != nullptr) {
                wchar_t buffer[2048]{};
                GetWindowTextW(GetDlgItem(hwnd, kInputId), buffer, static_cast<int>(std::size(buffer)));
                const std::string prompt = ToUtf8(buffer);

                SetWindowTextW(GetDlgItem(hwnd, kOutputId), L"Running...");
                state->inflight = std::make_unique<std::future<std::string>>(std::async(std::launch::async, [cfg = state->config, prompt]() {
                    return RunSingleRequest(cfg, prompt);
                }));
                SetTimer(hwnd, 1, 100, nullptr);
            }
            return 0;
        }
        case WM_TIMER: {
            if (state != nullptr && state->inflight != nullptr &&
                state->inflight->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                try {
                    const std::string output = state->inflight->get();
                    SetWindowTextW(GetDlgItem(hwnd, kOutputId), ToWide(output).c_str());
                } catch (const std::exception& ex) {
                    state->last_error = ex.what();
                    RefreshInfo(hwnd, *state);
                    SetWindowTextW(GetDlgItem(hwnd, kOutputId), ToWide(std::string("Error: ") + ex.what()).c_str());
                }
                state->inflight.reset();
                KillTimer(hwnd, 1);
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int RunGui(const RuntimeConfig& config) {
    const wchar_t klass[] = L"SqlBotWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = klass;

    RegisterClassW(&wc);

    GuiState state{ config, nullptr,{} };
    HWND hwnd = CreateWindowExW(0, klass, L"SQLBot", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, 760, 620,
                                nullptr, nullptr, GetModuleHandle(nullptr), &state);
    if (hwnd == nullptr) {
        throw std::runtime_error("Failed to initialize Win32 GUI");
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

} // namespace
#endif

} // namespace sqlbot

int main(int argc, const char** argv) {
    try {
        const sqlbot::RuntimeConfig cfg = sqlbot::LoadRuntimeConfig(argc, argv);

        if (cfg.gui_mode) {
#ifdef _WIN32
            return sqlbot::RunGui(cfg);
#else
            std::cerr << "GUI mode is only available on Windows builds.\n";
            return 2;
#endif
        }

        if (!cfg.nl_prompt.has_value()) {
            sqlbot::PrintUsage();
            return 1;
        }

        std::cout << sqlbot::RunSingleRequest(cfg, *cfg.nl_prompt) << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "sqlbot error: " << ex.what() << '\n';
        return 1;
    }
}
