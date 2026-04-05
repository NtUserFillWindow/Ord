#pragma once
#include <functional>
#include <optional>
#include <variant>

#include <Gdiplus.h>

namespace Lyra::UI::Foundation::Base {
class RenderableObject;
};

namespace Lyra::UI::Foundation::Events {
namespace EventPayloads {
// Todo: 后续这个结构体或被弃用，因为IME
struct KeyData {
    int  virtualKeyCode = NULL;
    bool isRepeat       = false;
};

struct MouseData {
    enum class MouseTypes : uint8_t {
        MouseMove              = 0b1111,
        LeftButtonUp           = 0b0000,
        LeftButtonDown         = 0b0001,
        LeftButtonDoubleDown   = 0b0010,
        RightButtonUp          = 0b0100,
        RightButtonDown        = 0b0101,
        RightButtonDoubleDown  = 0b0110,
        MiddleButtonUp         = 0b1000,
        MiddleButtonDown       = 0b1001,
        MiddleButtonDoubleDown = 0b1010,
    };

    Gdiplus::Point position = {};
    MouseTypes     type     = MouseTypes::MouseMove;
};
} // namespace EventPayloads

using EventPayload = std::variant<EventPayloads::MouseData, EventPayloads::KeyData>;

struct EventArgs {
    enum class EventTypes {
        None = NULL,
        KeyEvent,
        MouseEvent,
    };

    EventTypes              type    = EventTypes::None;
    Base::RenderableObject* target  = nullptr;
    EventPayload            payload = {};
    bool                    handled = false;
};

using EventCallback = std::function<void(const EventArgs&)>;
}; // namespace Lyra::UI::Foundation::Events