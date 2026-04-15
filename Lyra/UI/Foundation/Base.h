#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <vector>

#include <gsl/gsl>

#include "Events.h"
#include "Managers.h"

namespace Lyra::UI::Foundation::Base {
// ===== Object ====

class Object {
  public:
    uint32_t GetUniqueID() const { return _uniqueID; }
    void     AssignUniqueID(uint32_t id) {
        static std::vector<uint32_t> idPool{};
        if (std::find(idPool.rbegin(), idPool.rend(), id) != idPool.rend()) {
            throw std::exception("ID has already existed in the pool. It must be unique at all. ");
        }

        idPool.push_back(id);
        _uniqueID = id;
    }

  public:
    std::wstring_view Type = L"Object";

  private:
    uint32_t _uniqueID = std::numeric_limits<uint32_t>::max();
};

class RenderableObject : public Object {
  public:
    RenderableObject()          = default;
    virtual ~RenderableObject() = default;

    RenderableObject(const RenderableObject&)            = delete;
    RenderableObject& operator=(const RenderableObject&) = delete;
    RenderableObject(RenderableObject&&)                 = delete;
    RenderableObject& operator=(RenderableObject&&)      = delete;

  public:
    virtual RenderableObject* HitTest(const Gdiplus::Point&) = 0;
    virtual void              ProcessEvent(const Events::EventArgs& e) { e; }

    virtual bool Render(Foundation::Managers::Renderer& renderer) = 0;
    virtual bool PreRender(const RenderContext&)                  = 0;

    virtual void UpdateInteractBound(Gdiplus::Rect rect) { _interactBound = rect; };

  public:
    auto IsVisible() const { return _isVisible; }
    void SetVisible(bool visible) { _isVisible = visible; }

    auto IsPermeable() const { return _isPermeable; }
    void SetPermeable(bool permeable) { _isPermeable = permeable; }

    auto GetLayoutRect() const { return _layoutRect; }
    void SetLayoutRect(Gdiplus::Rect rect) {
        _layoutRect = rect;
        UpdateInteractBound(rect);
    }

  protected:
    bool BaseHitTest(const Gdiplus::Point& mousePosition) const { return _isVisible && !_isPermeable && _interactBound.Contains(mousePosition); }

  private:
    bool          _isVisible     = true;
    bool          _isPermeable   = false; // 实在找不到可替代的词汇了 orz
    Gdiplus::Rect _layoutRect    = {};
    Gdiplus::Rect _interactBound = {};
};

// ===== Node ====

struct NodeBase {
    std::uint32_t          zIndex   = 0;
    NodeBase*              parent   = nullptr;
    std::vector<NodeBase*> children = {};

    struct {
        NodeBase* prev = nullptr;
        NodeBase* next = nullptr;
    } sibling = {};

  public:
    NodeBase()          = default;
    virtual ~NodeBase() = default;

    NodeBase(const NodeBase&)            = delete;
    NodeBase& operator=(const NodeBase&) = delete;
    NodeBase(NodeBase&&)                 = delete;
    NodeBase& operator=(NodeBase&&)      = delete;

  public:
    auto& GetChildren() { return children; }
};

template <bool IsNestable>
struct Node : public NodeBase {
  public:
    bool Nestable() const { return IsNestable; }

    void Reparent(Node<true>* newParent, uint32_t zIndex = std::numeric_limits<uint32_t>::max()) {
        if (newParent == this) {
            return;
        }

        if (parent == newParent) {
            if (zIndex != std::numeric_limits<uint32_t>::max()) {
                SetZIndex(zIndex);
                if (parent) {
                    parent->Sort(false);
                }
            }
            return;
        }

        if (parent) {
            parent->RemoveChild(this);
        }

        if (newParent) {
            newParent->AppendChild(this, zIndex);
            return;
        }

        if (zIndex != std::numeric_limits<uint32_t>::max()) {
            SetZIndex(zIndex);
        }
    }

    template <bool Unused>
    void AppendChild(Node<Unused>* child, uint32_t zIndex = std::numeric_limits<uint32_t>::max())
    requires IsNestable
    {
        if constexpr (!IsNestable) {
            return;
        }

        if (!child || (child->Nestable() && (Node<true>*)child == this)) {
            return;
        }

        if (child->parent == this) {
            if (zIndex == std::numeric_limits<uint32_t>::max()) {
                return;
            }

            child->SetZIndex(zIndex);
            Sort(false);
            return;
        }

        if (child->parent) {
            ((Node<true>*)child->parent)->RemoveChild(child);
        }

        if (zIndex == std::numeric_limits<uint32_t>::max()) {
            if (children.empty()) {
                zIndex = 0;
            }

            else {
                zIndex = ((Node<true>*)children.back())->GetZIndex() + 1;
            }
        }

        child->SetZIndex(zIndex);

        children.push_back(child);
        child->parent = this;

        Sort(false);
    }

    template <bool Unused>
    void RemoveChild(Node<Unused>* child)
    requires IsNestable
    {
        if constexpr (!IsNestable) {
            return;
        }

        if (!child) {
            return;
        }

        auto it = std::find(children.begin(), children.end(), child);
        if (it == children.end()) {
            return;
        }

        (*it)->parent       = nullptr;
        (*it)->sibling.prev = nullptr;
        (*it)->sibling.next = nullptr;

        children.erase(it);
        RefreshChildrenLinks();
    }

    void Sort(bool recursive = true)
    requires IsNestable
    {
        std::stable_sort(children.begin(), children.end(), [](NodeBase* lhs, NodeBase* rhs) { return ((Node<true>*)lhs)->GetZIndex() < ((Node<true>*)rhs)->GetZIndex(); });

        RefreshChildrenLinks();

        if (recursive) {
            for (auto* child : children) {
                ((Node<true>*)child)->Sort();
            }
        }
    }

    uint32_t GetZIndex() const { return zIndex; }
    void     SetZIndex(int32_t index) {
        zIndex = index;

        if (parent) {
            ((Node<true>*)parent)->Sort(false);
        }
    }

  protected:
    void RefreshChildrenLinks() {
        for (std::size_t index = 0; index < children.size(); ++index) {
            auto* curr         = children[index];
            curr->parent       = this;
            curr->sibling.prev = (index > 0) ? children[index - 1] : nullptr;
            curr->sibling.next = (index + 1 < children.size()) ? children[index + 1] : nullptr;
        }
    }
};

struct PreRenderContext {
    Foundation::Managers::Renderer* renderer  = nullptr;
    Gdiplus::Rect                   dirtyRect = {};
};

template <bool IsNestable = false>
class RenderableNode : public Node<IsNestable>, public RenderableObject {
  public:
    bool PreRender(const RenderContext& context) override {
        if (!IsVisible()) {
            return false;
        }

        auto&       renderer   = *context.renderer;
        auto&       graphics   = renderer.AllocGraphics();
        const auto& targetRect = GetLayoutRect();

        Gdiplus::GraphicsState state;
        const auto             pGraphics = graphics.GetGraphics();
        Native::DllExports::GdipSaveGraphics(pGraphics, &state);
        Native::DllExports::GdipTranslateWorldTransform(pGraphics, targetRect.X * 1.f, targetRect.Y * 1.f, Gdiplus::MatrixOrderAppend);
        Native::DllExports::GdipSetClipRectI(pGraphics, 0, 0, targetRect.Width, targetRect.Height, Gdiplus::CombineModeIntersect);

        Render(renderer);

        if constexpr (!IsNestable) {
            Native::DllExports::GdipRestoreGraphics(pGraphics, state);
            return true;
        }

        for (auto* child : this->GetChildren()) {
            auto       node = (RenderableNode<true>*)(Node<true>*)child;
            const auto rect = node->GetLayoutRect();

#ifdef _DEBUG
            OutputDebugStringW(
                std::format(
                    L"Object: {} | Component id - {} | RenderDirtyStatus: {}\r\n", node->Type, node->GetUniqueID(), rect.IntersectsWith(context.dirtyRect) ? L"Retained" : L"Skipped"
                )
                    .c_str()
            );
#endif

            if (!rect.IntersectsWith(context.dirtyRect)) {
                continue;
            }

            node->PreRender(context);
        }

        Native::DllExports::GdipRestoreGraphics(pGraphics, state);
        return true;
    }

    RenderableNode* HitTest(const Gdiplus::Point& screenPos) override {
        if (!BaseHitTest(screenPos)) {
            return nullptr;
        }

        if constexpr (IsNestable) {
            const std::vector<NodeBase*>& children     = this->GetChildren();
            RenderableNode*               targetObject = nullptr;

            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if (auto child = *it) {
                    auto node = static_cast<Node<IsNestable>*>(child);

                    targetObject = (static_cast<RenderableNode*>(node))->HitTest(screenPos);
                }

                if (targetObject) {
                    return targetObject;
                }
            }
        }

        return this;
    }
};
} // namespace Lyra::UI::Foundation::Base