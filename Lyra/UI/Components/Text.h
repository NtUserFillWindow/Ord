#pragma once
#include "../Foundation/Base.h"

namespace Lyra::UI::Components {
class Text : public Foundation::Base::RenderableNode<false> {
  public:
    Text()                       = default;
    Text(Text&&)                 = delete;
    Text(const Text&)            = delete;
    Text& operator=(Text&&)      = delete;
    Text& operator=(const Text&) = delete;

    bool Render(Foundation::Managers::Renderer& renderer) override {
        auto& graphics = renderer.AllocGraphics();

        // graphics.Clear(Gdiplus::Color::Transparent);
        graphics.Clear(Gdiplus::Color::Red);
        DrawBackground(renderer);
        const auto&         font = Foundation::Managers::FontManager::Instance().GetFont(L"Segoe UI", 24);
        Gdiplus::SolidBrush brush(Gdiplus::Color::White);

        graphics.DrawString(_content.c_str(), (int)_content.length(), font.get(), Gdiplus::PointF(.0f, .0f), &brush);
        return true;
    };

    auto GetContent() const { return _content; }
    void SetContent(const std::wstring& content) { _content = content; }

  private:
    std::wstring _content = L"文本";
};
} // namespace Lyra::UI::Components