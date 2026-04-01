#pragma once
#define NOMINMAX

#include <memory>
#include <string>
#include <unordered_map>

#include "UI/Startup.h"
#include "UI/Components/Window.h"

namespace Lyra {
class WindowFoundation;

static inline HINSTANCE                         g_processInstance{};
static inline std::shared_ptr<WindowFoundation> g_ownerWindow{};
static inline UINT_PTR                          g_gdiplusToken{};

class WindowFoundation {
  private:
    inline static ATOM _CommonClassAtom = 0;

  public:
    WindowFoundation() : _selfLayout(new UI::Components::Window) { Initialize(); }
    ~WindowFoundation() { Uninitialize(); }
    bool Create(const std::wstring& title) {
        if (!CreateNative(title)) {
            return false;
        }

        _selfLayout->BindNativeHandle(nativeHandle);
        return true;
    }
    bool Activate() const {
        if (!nativeHandle) {
            return false;
        }

        ShowWindow(nativeHandle, SW_SHOW);
        UpdateWindow(nativeHandle);
        return true;
    }

  private:
    void Initialize() {
        WNDCLASSEXW classInfo{};
        classInfo.cbSize        = sizeof(WNDCLASSEXW);
        classInfo.style         = CS_HREDRAW | CS_VREDRAW; // Here must be deleted after logic optimized
        classInfo.lpfnWndProc   = WindowFoundation::WindowProcedure;
        classInfo.cbClsExtra    = 0;
        classInfo.cbWndExtra    = 0;
        classInfo.hInstance     = g_processInstance;
        classInfo.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
        classInfo.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        classInfo.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        classInfo.lpszMenuName  = nullptr;
        classInfo.lpszClassName = L"LyraUniversalWindowClass";
        classInfo.hIconSm       = nullptr;

        _CommonClassAtom = RegisterClassExW(&classInfo);
    }
    void Uninitialize() {
        if (_CommonClassAtom) {
            UnregisterClassW(MAKEINTATOM(_CommonClassAtom), g_processInstance);
            _CommonClassAtom = 0;
        }
    }
    bool CreateNative(const std::wstring& title) {
        if (_CommonClassAtom == 0) {
            abort(); // Failed to create window due to class not registered.
            return false;
        }

        nativeHandle = CreateWindowExW(
            0,
            MAKEINTATOM(_CommonClassAtom),
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            nullptr,
            nullptr,
            g_processInstance,
            this
        );
        return nativeHandle != nullptr;
    }

    bool HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, _Out_ LRESULT& outResult) {
        outResult = 0;

        if (uMsg == WM_SIZE) {
            _selfLayout->UpdateSize(lParam);
            return true;
        }

        if (uMsg == WM_PAINT) {
            { // I don't know whether I could remove this block.
                PAINTSTRUCT paintStruct{};
                const auto  hdc = ::BeginPaint(nativeHandle, &paintStruct);
                ::EndPaint(nativeHandle, &paintStruct);
            }

            if (_selfLayout->Render()) {
                _selfLayout->Present();
            }
        }

        if (uMsg == WM_ERASEBKGND) {
            return true;
        }

        if (uMsg == WM_NCCALCSIZE) {
            return true;
        }

        return false;
    }

    static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // HACK: TLS??
        static std::unordered_map<HWND, WindowFoundation*> instanceMap{};
        static HWND                                        ownerHandle;

        LRESULT result = 0;
        if (uMsg == WM_NCCREATE) {
            const auto createStruct = (CREATESTRUCTW*)lParam;
            const auto self         = (WindowFoundation*)createStruct->lpCreateParams;

            instanceMap.emplace(hWnd, self);
            if (instanceMap.size() == 1) {
                ownerHandle = hWnd;
            }
            self->HandleMessage(uMsg, wParam, lParam, result);
            return TRUE;
        }

        const auto it = instanceMap.find(hWnd);
        if (it != instanceMap.end()) {
            if (uMsg == WM_NCDESTROY) {
                it->second->HandleMessage(uMsg, wParam, lParam, result);

                instanceMap.erase(hWnd);
                if (ownerHandle == hWnd) {
                    ownerHandle = nullptr;
                }

                if (instanceMap.empty() || ownerHandle == nullptr) {
                    ::PostQuitMessage(0);
                }
                return 0;
            } else {
                if (it->second->HandleMessage(uMsg, wParam, lParam, result)) {
                    return result;
                }
            }
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

  public:
    std::unique_ptr<UI::Components::Window> _selfLayout{};

  private:
    HWND nativeHandle{};
};

void Initialize(HINSTANCE processInstance) {
    g_processInstance = processInstance;
    Gdiplus::GdiplusStartupInput input{};
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &input, nullptr);
}
void Uninitialize() {
    Lyra::UI::Foundation::Managers::FontManager::Instance().Clear();

    Gdiplus::GdiplusShutdown(g_gdiplusToken);
    g_gdiplusToken    = NULL;
    g_processInstance = nullptr;
}
void Run(std::shared_ptr<WindowFoundation>& ownerWindow) {
    if (!ownerWindow) {
        // It seems passing invalid parameters.
        abort();
    }

    g_ownerWindow = ownerWindow;

    MSG  message{};
    BOOL status = FALSE;

    while ((status = GetMessageW(&message, nullptr, 0, 0)) != 0) {
        if (status == -1) {
            abort();
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
};
} // namespace Lyra