#pragma once
namespace Lyra::UI::Components {
class Window;
};

namespace Lyra::UI::Foundation::Managers {
class Renderer final {
  public:
    Renderer()                           = default;
    Renderer(Renderer&&)                 = delete;
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(Renderer&&)      = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer()                          = default;

    Gdiplus::Graphics& AllocGraphics() {
        if (!_graphics) {
            _graphics = std::make_shared<Gdiplus::Graphics>(_swapchain.GetSwapchainContext());
        }

        return *_graphics;
    };

    void Invalidate(const Gdiplus::Rect invalidatedRect) const {
        RECT rect{};
        rect.left   = invalidatedRect.X;
        rect.top    = invalidatedRect.Y;
        rect.right  = invalidatedRect.GetRight();
        rect.bottom = invalidatedRect.GetBottom();
        InvalidateRect(_swapchain.GetTargetWindow(), &rect, FALSE);
    }

  private:
    friend class Components::Window;

    void ApplyWindow(HWND windowHandle) { _swapchain.BindToWindow(windowHandle); };

    bool Present() { return _swapchain.PresentBuffer(); };
    void UpdateSize(LPARAM lParam) {
        _swapchain.UpdateSize(lParam);

        if (_swapchain.Invalid()) {
            return;
        }

        _graphics = std::make_shared<Gdiplus::Graphics>(_swapchain.GetSwapchainContext());
    };

  private:
    Native::Swapchain                  _swapchain = {};
    std::shared_ptr<Gdiplus::Graphics> _graphics  = nullptr;
};

// Font Management

struct FontDescriptor {
    std::wstring       family = L"Segoe UI";
    float              size   = 24.0f;
    Gdiplus::FontStyle style  = Gdiplus::FontStyleRegular;

    bool operator<(const FontDescriptor& other) const {
        return std::tie(family, size, style) < std::tie(other.family, other.size, other.style);
    }

    bool operator==(const FontDescriptor& other) const {
        return family == other.family && size == other.size && style == other.style;
    }
};

class FontManager final {
  public:
    using FontCache = std::shared_ptr<Gdiplus::Font>;

  public:
    FontManager(FontManager&&)                 = delete;
    FontManager(const FontManager&)            = delete;
    FontManager& operator=(FontManager&&)      = delete;
    FontManager& operator=(const FontManager&) = delete;
    ~FontManager()                             = default;

    static FontManager& Instance() {
        static FontManager _instance{};
        return _instance;
    }

    void Clear() {
        _fontCache.clear();
        _aliases.clear();
    }

    void RegisterAlias(const std::string& alias, const FontDescriptor& desc) { _aliases[alias] = desc; }
    void RegisterAlias(
        const std::string&  alias,
        const std::wstring& fontFamily,
        float               size,
        Gdiplus::FontStyle  style = Gdiplus::FontStyleRegular
    ) {
        _aliases[alias] = {fontFamily, size, style};
    }

    FontCache GetFont(const FontDescriptor& desc) {
        const auto it = _fontCache.find(desc);
        if (it != _fontCache.end()) {
            return it->second;
        }

        auto font = std::make_shared<Gdiplus::Font>(desc.family.c_str(), desc.size, desc.style);
        if (font->GetLastStatus() != Gdiplus::Ok) {
            font = std::make_shared<Gdiplus::Font>(L"Segoe UI", 24.0f, Gdiplus::FontStyleRegular);
        }

        _fontCache[desc] = font;
        return font;
    }
    FontCache
    GetFont(const std::wstring& fontFamily, float size, Gdiplus::FontStyle style = Gdiplus::FontStyleRegular) {
        return GetFont({fontFamily, size, style});
    }
    FontCache GetFont(const std::string& alias) {
        const auto it = _aliases.find(alias);
        if (it != _aliases.end()) {
            return GetFont(it->second);
        }

        return GetFont(L"Segoe UI", 24);
    };

  private:
    FontManager() = default;

    std::map<FontDescriptor, FontCache>             _fontCache = {};
    std::unordered_map<std::string, FontDescriptor> _aliases   = {};
};
} // namespace Lyra::UI::Foundation::Managers