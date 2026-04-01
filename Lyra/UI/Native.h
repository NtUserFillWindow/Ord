#pragma once
#define GDIPVER 0x0110 // NOSONAR, this is determined by Win32 Header. It's very bad

#include <sal.h>
#include <Windows.h>
#include <Windowsx.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#undef GDIPVER

namespace Lyra::UI::Native {
class Swapchain final {
  public:
    Swapchain()                            = default;
    Swapchain(Swapchain&&)                 = delete;
    Swapchain(const Swapchain&)            = delete;
    Swapchain& operator=(Swapchain&&)      = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    ~Swapchain() {
        if (_virtualDefaultBufferHandle) {
            SelectBitmap(_virtualBufferContext, _virtualDefaultBufferHandle);
            _virtualDefaultBufferHandle = nullptr;
        }

        if (_virtualBufferHandle) {
            DeleteBitmap(_virtualBufferHandle);
            _virtualBufferHandle = nullptr;
        }

        if (_virtualBufferContext) {
            ::DeleteDC(_virtualBufferContext);
            _virtualBufferContext = nullptr;
        }

        if (_windowDeviceContext) {
            ::ReleaseDC(_windowHandle, _windowDeviceContext);
            _windowHandle        = nullptr;
            _windowDeviceContext = nullptr;
        }
    }

    bool Invalid() const { return _virtualBufferContext == nullptr; }

    void BindToWindow(HWND hWnd) {
        if (!::IsWindow(hWnd)) {
            return;
        }

        if (hWnd == _windowHandle) {
            throw std::exception("Incorrect calling method. ");
        }

        _windowHandle         = hWnd;
        _windowDeviceContext  = ::GetDC(hWnd);
        _virtualBufferContext = ::CreateCompatibleDC(nullptr);
        UpdateSize(NULL);
    }
    void UpdateSize(LPARAM lParam = NULL) {
        if (lParam == NULL) {
            RECT rect{};
            ::GetWindowRect(_windowHandle, &rect);
            _windowSize = {rect.right - rect.left, rect.bottom - rect.top};
        }

        else {
            _windowSize = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        }

        if (_virtualBufferHandle) {
            DeleteBitmap(_virtualBufferHandle);
            _virtualBufferHandle = nullptr;
        };

        _virtualBufferHandle = _CreateMemoryBuffer();

        if (_virtualDefaultBufferHandle) {
            SelectBitmap(_virtualBufferContext, _virtualBufferHandle);
        }

        else {
            _virtualDefaultBufferHandle = SelectBitmap(_virtualBufferContext, _virtualBufferHandle);
        };
    };

    bool PresentBuffer() const {
        if (!_windowDeviceContext || !_virtualBufferContext) {
            return false;
        }

        return ::BitBlt(
            _windowDeviceContext, 0, 0, _windowSize.cx, _windowSize.cy, _virtualBufferContext, 0, 0, SRCCOPY
        );
    };

    auto GetTargetWindow() const { return _windowHandle; };
    auto GetSwapchainSize() const { return _windowSize; };
    auto GetSwapchainContext() const {
        if (_virtualBufferContext == nullptr) {
            // if encounter this abort, it means you are trying
            // to get the buffer before calling BindToWindow successfully.
            abort();
        }

        return _virtualBufferContext;
    }

  private:
    HBITMAP _CreateMemoryBuffer() const {
        if (!_windowDeviceContext) {
            return nullptr;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = _windowSize.cx;
        bmi.bmiHeader.biHeight      = _windowSize.cy;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr; // I throw it away~~~~~~
        return CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    }

  private:
    SIZE    _windowSize                 = {};
    HWND    _windowHandle               = nullptr;
    HDC     _windowDeviceContext        = nullptr;
    HDC     _virtualBufferContext       = nullptr;
    HBITMAP _virtualBufferHandle        = nullptr;
    HBITMAP _virtualDefaultBufferHandle = nullptr;
};
} // namespace Lyra::UI::Native