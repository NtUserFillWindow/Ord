#pragma once
#include <numeric>

#include "../Foundation/Base.h"
#include "Declare.h"

namespace Lyra::UI::Components {
class Layout : public Foundation::Base::RenderableNode<true> {
  public:
    using Point      = std::array<uint16_t, 2>;
    using LayoutData = std::vector<Point>;

    Layout() { Type = L"Object.Renderable.Layout"; }

    bool Render(Foundation::Managers::Renderer&) { return true; };
    bool PreRender(Foundation::Managers::Renderer& renderer) {
        if (children.empty() || _layout.empty()) {
            return false;
        }

        if (_isDirty) {
            RecalculateComponentPosition();
            _isDirty = false;
        }

        auto&      graphics = renderer.AllocGraphics();
        auto       state    = graphics.Save();
        const auto selfRect = GetLayoutRect();

        graphics.TranslateTransform((float_t)selfRect.X, (float_t)selfRect.Y);
        graphics.IntersectClip(Gdiplus::Rect(0, 0, selfRect.Width, selfRect.Height));
        Render(renderer);
        graphics.Restore(state);

        for (auto& child : children) {
            auto* node = (RenderableNode*)child;

            const auto childState = graphics.Save();
            node->PreRender(renderer);
            graphics.Restore(childState);
        }
        return true;
    }

    void SetLayout(const LayoutData& layout) {
        _layout  = layout;
        _isDirty = true;
    }
    void SetLayout(LayoutData&& layout) noexcept {
        _layout  = std::move(layout);
        _isDirty = true;
    }
    void SetLayout(std::initializer_list<Point> layout) {
        _layout.assign(layout.begin(), layout.end());
        _isDirty = true;
    }

    // void SetLayoutRect() {
    //}

    void SetAlign(Align x, Align y) noexcept {
        _alignX  = x;
        _alignY  = y;
        _isDirty = true;
    }
    void SetHorizontalAlign(Align x) noexcept {
        _alignX  = x;
        _isDirty = true;
    }
    void SetVerticalAlign(Align y) noexcept {
        _alignY  = y;
        _isDirty = true;
    }

  private:
    void RecalculateComponentPosition() {
        if (children.empty() || _layout.empty()) {
            return;
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
            const auto& rect  = child->GetLayoutRect();

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

        const auto selfRect = GetLayoutRect();
        int32_t    x        = 0;
        int32_t    y        = 0;

        if (_alignX & Align::Center) x = (int32_t)((selfRect.Width - requiredWidth) * 0.5f);
        else if (_alignX & Align::End) x = selfRect.Width - requiredWidth;
        if (_alignY & Align::Center) y = (int32_t)((selfRect.Height - requiredHeight) * 0.5f);
        else if (_alignY & Align::End) y = selfRect.Height - requiredHeight;

        std::vector<int32_t> colStartX(maxCol + 1, 0);
        for (uint16_t c = 1; c <= maxCol; ++c) {
            colStartX[c] = colStartX[(size_t)c - 1] + colWidths[(size_t)c - 1] + _itemGap;
        }

        std::vector<int32_t> rowStartY(maxRow + 1, 0);
        for (uint16_t r = 1; r <= maxRow; ++r) {
            rowStartY[r] = rowStartY[(size_t)r - 1] + rowHeights[(size_t)r - 1] + _rowGap;
        }

        for (size_t i = 0; i < max; ++i) {
            const auto col   = _layout[i][0];
            const auto row   = _layout[i][1];
            auto*      child = (RenderableNode*)children[i];

            auto childRect = child->GetLayoutRect();
            childRect.X    = x + colStartX[col];
            childRect.Y    = y + rowStartY[row];

            child->SetLayoutRect(childRect);
        }
    }

  private:
    LayoutData _layout  = {};
    bool       _isDirty = false;
    int32_t    _itemGap = 20;
    int32_t    _rowGap  = 0;
    Align      _alignX  = {Align::Start};
    Align      _alignY  = {Align::Start};
};
} // namespace Lyra::UI::Components