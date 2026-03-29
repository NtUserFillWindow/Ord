#pragma once
#include "../Foundation.h"
#include "Button.h"
#include "Layout.h"

namespace Lyra::UI::Components {
class Window : public Foundation::RenderableNode<true> {
  public:
    Window(Window&&)                 = default;
    Window(const Window&)            = delete;
    Window& operator=(Window&&)      = default;
    Window& operator=(const Window&) = delete;

  public:
    Window() {
        gsl::owner<Layout*> layout = new Layout{};
        layout->SetHorizontalAlign(Align::End);
        layout->SetObjectRect({});
        layout->SetLayout({
            {0, 0},
            {1, 0},
            {2, 0}
        });

        for (uint8_t i = 0; i < 3; i++) {
            gsl::owner<Button*> button = new Button{};
            button->SetObjectRect({0, 0, 48, 30});
            layout->AppendChild(button);
        }

        AppendChild(layout);
    }
    ~Window() = default;

    void BindNativeHandle(HWND windowHandle) { renderer.ApplyWindow(windowHandle); }
    void UpdateSize(LPARAM lParam) {
        renderer.UpdateSize(lParam);
        SetObjectRect({0, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
        ((RenderableNode*)children[0])
            ->SetObjectRect({0, 0, GET_X_LPARAM(lParam), 30}); // zindex-0 means layout component.
    }
    void Present() { renderer.Present(); }

    bool Render(Foundation::Renderer& renderer) override {
        auto& graphics = renderer.AllocGraphics();
        graphics.Clear(Gdiplus::Color(0xFF262626));
        return true;
    };
    bool Render() { return PreRender(renderer); }

  public:
    Foundation::Renderer renderer{};
};
} // namespace Lyra::UI::Components