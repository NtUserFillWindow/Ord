#pragma once
#include "Text.h"

namespace Lyra::UI::Components {
class Button : public Text {
  public:
    Button() { Type = L"Object.Renderable.Button"; }
};
} // namespace Lyra::UI::Components