#pragma once
#include <numeric>

#include "../Foundation/Base.h"

namespace Lyra::UI::Components {
enum class Align : std::uint8_t {
    Start  = 0b00,
    Center = 0b01,
    End    = 0b10
};

uint8_t operator&(Align lhs, Align rhs) {
    return (uint8_t)lhs & (uint8_t)rhs;
}

class Layout : public Foundation::Base::RenderableNode<true> {
  public:
    using Point      = std::array<uint16_t, 2>;
    using LayoutData = std::vector<Point>;

    bool Render(Foundation::Managers::Renderer& renderer) { return true; };
    bool PreRender(Foundation::Managers::Renderer& renderer) {
        if (children.empty() || _layout.empty()) {
            return false;
        }

        uint16_t maxRow = 0;
        uint16_t maxCol = 0;
        for (const auto& point : _layout) {
            maxRow = std::max(maxRow, point[1]);
            maxCol = std::max(maxCol, point[0]);
        }

        std::vector<int32_t> colWidths(maxCol + 1, 0);
        std::vector<int32_t> rowWidths(maxRow + 1, 0);
        std::vector<int32_t> rowHeights(maxRow + 1, 0);

        auto max = std::min(children.size(), _layout.size());
        for (size_t i = 0; i < max; ++i) {
            const auto col = _layout[i][0];
            const auto row = _layout[i][1];

            auto*       child = static_cast<RenderableNode*>(children[i]);
            const auto& rect  = child->GetObjectRect();

            colWidths[col]  = std::max(colWidths[col], rect.Width);
            rowHeights[row] = std::max(rowHeights[row], rect.Height);
        }

        for (uint16_t row = 0; row <= maxRow; ++row) {
            int32_t width = std::accumulate(colWidths.begin(), colWidths.end(), 0);
            if (maxCol > 0) {
                width += _itemGap * maxCol;
            }

            rowWidths[row] = width;
        }

        int32_t requiredWidth  = *std::max_element(rowWidths.begin(), rowWidths.end());
        int32_t requiredHeight = std::accumulate(rowHeights.begin(), rowHeights.end(), 0);
        if (maxRow > 0) {
            requiredHeight += _rowGap * maxRow;
        }

        const auto selfRect = GetObjectRect();
        float_t    x        = 0;
        float_t    y        = 0;

        if (_alignX & Align::Center) x = (GetObjectRect().Width - requiredWidth) * 0.5f;
        else if (_alignX & Align::End) x = (float_t)(GetObjectRect().Width - requiredWidth);
        if (_alignY & Align::Center) y = (GetObjectRect().Height - requiredHeight) * 0.5f;
        else if (_alignY & Align::End) y = (float_t)(GetObjectRect().Height - requiredHeight);

        auto& graphics = renderer.AllocGraphics();
        auto  state    = graphics.Save();

        graphics.TranslateTransform((float_t)selfRect.X, (float_t)selfRect.Y);
        graphics.IntersectClip(Gdiplus::Rect(0, 0, selfRect.Width, selfRect.Height));
        Render(renderer);
        graphics.Restore(state);

        state = graphics.Save();
        graphics.TranslateTransform(x, y);
        graphics.IntersectClip(Gdiplus::Rect(0, 0, requiredWidth, requiredHeight));

        std::vector<float_t> colStartX(maxCol + 1, 0);
        for (uint16_t c = 1; c <= maxCol; ++c) {
            colStartX[c] = colStartX[(size_t)c - 1] + colWidths[(size_t)c - 1] + _itemGap;
        }

        std::vector<float_t> rowStartY(maxRow + 1, 0);
        for (uint16_t r = 1; r <= maxRow; ++r) {
            rowStartY[r] = rowStartY[(size_t)r - 1] + rowHeights[(size_t)r - 1] + _rowGap;
        }

        for (size_t i = 0; i < max; ++i) {
            const auto col   = _layout[i][0];
            const auto row   = _layout[i][1];
            auto*      child = (RenderableNode*)children[i];

            const auto childState = graphics.Save();
            graphics.TranslateTransform(colStartX[col], rowStartY[row]);
            child->PreRender(renderer);
            graphics.Restore(childState);
        }

        graphics.Restore(state);
        return true;
    }

    void SetLayout(const LayoutData& layout) { _layout = layout; }
    void SetLayout(LayoutData&& layout) noexcept { _layout = std::move(layout); }
    void SetLayout(std::initializer_list<Point> layout) { _layout.assign(layout.begin(), layout.end()); }

    void SetAlign(Align x, Align y) noexcept {
        _alignX = x;
        _alignY = y;
    }
    void SetHorizontalAlign(Align x) noexcept { _alignX = x; }
    void SetVerticalAlign(Align y) noexcept { _alignY = y; }

  private:
    LayoutData _layout  = {};
    int32_t    _itemGap = 20;
    int32_t    _rowGap  = 0;
    Align      _alignX  = {Align::Start};
    Align      _alignY  = {Align::Start};
};
} // namespace Lyra::UI::Components