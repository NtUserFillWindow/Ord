#pragma once
#include "../Foundation.h"

namespace Lyra::UI::Components {
class Button : public Foundation::RenderableNode<false> {
  public:
    Button()                         = default;
    Button(Button&&)                 = delete;
    Button(const Button&)            = delete;
    Button& operator=(Button&&)      = delete;
    Button& operator=(const Button&) = delete;

    bool Render(Foundation::Renderer& renderer) override {
        auto& graphics = renderer.AllocGraphics();

        graphics.Clear(Gdiplus::Color::Red);
        Gdiplus::Font       font(L"Segoe UI", 24);
        Gdiplus::SolidBrush brush(Gdiplus::Color::White);
        graphics.DrawString(L"Hello, Lyra!", 13, &font, Gdiplus::PointF(.0f, .0f), &brush);
        return true;
    };
};
} // namespace Lyra::UI::Components