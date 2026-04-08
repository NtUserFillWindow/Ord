#pragma once
#include <string>

#include "../Foundation/Base.h"
#include "Declare.h"

namespace Lyra::UI::Components {
class Text : public Foundation::Base::RenderableNode<false> {
  public:
    Text(Text&&)                 = delete;
    Text(const Text&)            = delete;
    Text& operator=(Text&&)      = delete;
    Text& operator=(const Text&) = delete;
    ~Text()                      = default;

  public:
    Text() { Type = L"Object.Renderable.Text"; }

    bool Render(Foundation::Managers::Renderer& renderer) override {
        auto& graphics = renderer.AllocGraphics();

        Gdiplus::StringFormat stringFormat;
        if (_alignX & Align::Center) stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
        else if (_alignX & Align::End) stringFormat.SetAlignment(Gdiplus::StringAlignmentFar);
        if (_alignY & Align::Center) stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        else if (_alignY & Align::End) stringFormat.SetLineAlignment(Gdiplus::StringAlignmentFar);

        const auto&         font = Foundation::Managers::FontManager::Instance().GetFont(L"Segoe UI", 16);
        Gdiplus::SolidBrush fontBrush(_fontColor);
        Gdiplus::SolidBrush backgroundBrush(_backgroundColor);

        const auto& rect = GetLayoutRect();
        graphics.FillRectangle(&backgroundBrush, rect);
        graphics.DrawString(
            _content.c_str(),
            (int32_t)_content.size(),
            font.get(),
            Gdiplus::RectF(0.f, 0.f, rect.Width * 1.f, rect.Height * 1.f),
            &stringFormat,
            &fontBrush
        );
        return true;
    };

    auto GetContent() const { return _content; }
    void SetContent(const std::wstring& content) { _content = content; }

    void SetAlign(Align x, Align y) noexcept {
        _alignX = x;
        _alignY = y;
    }
    void SetHorizontalAlign(Align x) noexcept { _alignX = x; }
    void SetVerticalAlign(Align y) noexcept { _alignY = y; }

  private:
    Gdiplus::Color _fontColor       = Gdiplus::Color::White;
    Gdiplus::Color _backgroundColor = Gdiplus::Color::Transparent;
    std::wstring   _content         = L"文本";
    Align          _alignX          = Align::Center;
    Align          _alignY          = Align::Center;
};
} // namespace Lyra::UI::Components