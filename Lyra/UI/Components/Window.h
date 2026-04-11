#pragma once
#include "../Foundation/Base.h"
#include "Text.h"
#include "Button.h"
#include "Layout.h"

namespace Lyra::UI::Components {
class Window : public Foundation::Base::RenderableNode<true> {
  public:
    Window(Window&&)                 = default;
    Window(const Window&)            = delete;
    Window& operator=(Window&&)      = default;
    Window& operator=(const Window&) = delete;

  public:
    Window() {
        Type = L"Object.Renderable.Window";
        InitializeComponents();
    }
    ~Window() = default;

    void BindNativeHandle(HWND windowHandle) { renderer.ApplyWindow(windowHandle); }
    void UpdateSize(LPARAM lParam) {
        const auto width  = GET_X_LPARAM(lParam);
        const auto height = GET_Y_LPARAM(lParam);

        renderer.UpdateSize(lParam);
        SetLayoutRect({0, 0, width, height});
        ((RenderableNode*)children[0])->SetLayoutRect({0, 0, width, 30}); // zindex-0 means layout component.
    }

    void Present() { renderer.Present(); }
    bool Render(Foundation::Managers::Renderer& renderer) override {
        auto& graphics = renderer.AllocGraphics();
        graphics.Clear(Gdiplus::Color(0xFF262626));

        return true;
    }
    bool Render() { return PreRender(renderer); }

    void RegisterEventCallback(Foundation::Events::EventCallback callback) {
        _eventCallbacks.push_back(std::move(callback));
    }
    void DispatchEvent(
        Foundation::Events::EventArgs::EventTypes type,
        const Foundation::Events::EventPayload&   payload,
        Gdiplus::Point                            mousePosition
    ) {
        Foundation::Events::EventArgs e{};
        e.type    = type;
        e.payload = payload;
        e.handled = false;

        e.target = HitTest(mousePosition);
        if (!e.target) {
            e.target = this;
        }

        auto a = ((RenderableNode*)e.target)->GetUniqueID();
        (int)std::sin(0);

        for (auto& callback : _eventCallbacks) {
            callback(e);
            if (e.handled) {
                break;
            }
        }

        if (e.target && !e.handled) {
            e.target->ProcessEvent(e);
        }
    }

  private:
    void InitializeComponents() {
        gsl::owner<Layout*> layout = new Layout{};

        for (uint8_t i = 0; i < 3; i++) {
            gsl::owner<Button*> button = new Button{};
            button->SetLayoutRect({0, 0, 48, 30});
            button->SetContent(L"按钮 " + std::to_wstring(i));
            button->AppendUniqueID(i + 1);
            layout->AppendChild(button);
        }
        gsl::owner<Text*> text = new Text{};
        text->SetLayoutRect({0, 0, 200, 30});
        text->SetContent(L"新建vb窗口1");
        text->SetPermeable(true);

        layout->SetHorizontalAlign(Align::End);
        layout->SetLayoutRect({});
        layout->SetLayout({
            {0, 0},
            {1, 0},
            {2, 0}
        });

        AppendChild(layout);
        AppendChild(text);
    };

  public:
    Foundation::Managers::Renderer renderer{};

  private:
    constexpr static uint32_t                      IDMinimizeButton = 1;
    constexpr static uint32_t                      IDMaximizeButton = 2;
    constexpr static uint32_t                      IDCloseButton    = 3;
    std::vector<Foundation::Events::EventCallback> _eventCallbacks{};
}; // namespace Lyra::UI::Components
} // namespace Lyra::UI::Components