#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <vector>

#include <gsl/gsl>

#include "../Native.h"
#include "./Events.h"
#include "./Managers.h"

namespace Lyra::UI::Foundation::Base {
class Object {
  public:
    std::wstring_view Type = L"Object";
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

    virtual bool Render(Foundation::Managers::Renderer& renderer)    = 0;
    virtual bool PreRender(Foundation::Managers::Renderer& renderer) = 0;

    virtual void UpdateInteractBound(Gdiplus::Rect rect) { _interactBound = rect; };

  public:
    auto IsVisible() const { return _isVisible; }
    void SetVisible(bool visible) { _isVisible = visible; }

    auto GetLayoutRect() const { return _layoutRect; }
    void SetLayoutRect(Gdiplus::Rect rect) {
        _layoutRect = rect;
        UpdateInteractBound(rect);
    }

  protected:
    bool BaseHitTest(const Gdiplus::Point& mousePosition) const {
        return _isVisible && _interactBound.Contains(mousePosition);
    }

  private:
    bool          _isVisible     = true;
    Gdiplus::Rect _layoutRect    = {};
    Gdiplus::Rect _interactBound = {};
};

struct NodeBase {
  public:
    NodeBase()          = default;
    virtual ~NodeBase() = default;

    NodeBase(const NodeBase&)            = delete;
    NodeBase& operator=(const NodeBase&) = delete;
    NodeBase(NodeBase&&)                 = delete;
    NodeBase& operator=(NodeBase&&)      = delete;

  public:
    std::uint32_t          zIndex   = 0;
    NodeBase*              parent   = nullptr;
    std::vector<NodeBase*> children = {};

    struct {
        NodeBase* prev = nullptr;
        NodeBase* next = nullptr;
    } sibling = {};

  public:
    virtual bool Nestable() const = 0;

    auto& GetChildren() { return children; }

    virtual void AppendChild(NodeBase* child, uint32_t z = std::numeric_limits<uint32_t>::max()) = 0;
    virtual void RemoveChild(NodeBase* child)                                                    = 0;
    virtual void Sort(bool recursive = true)                                                     = 0;

    void Reparent(NodeBase* newParent, uint32_t z = std::numeric_limits<uint32_t>::max()) {
        if (newParent == this) {
            return;
        }

        if (parent == newParent) {
            if (z != std::numeric_limits<uint32_t>::max()) {
                SetZIndex(z);
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
            newParent->AppendChild(this, z);
            return;
        }

        if (z != std::numeric_limits<uint32_t>::max()) {
            SetZIndex(z);
        }
    }

    uint32_t GetZIndex() const { return zIndex; }
    void     SetZIndex(int32_t index) {
        zIndex = index;

        if (parent) {
            parent->Sort(false);
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

  private:
    // Reserved Code
    template <class Pred>
    std::vector<NodeBase*> Serialize(Pred const& pred) {
        std::vector<NodeBase*> result;
        _SerializeInto(pred, result);
        return result;
    }

    template <class Pred>
    void _SerializeInto(Pred const& pred, std::vector<NodeBase*>& out) {
        if (pred(this)) {
            out.push_back(this);
        }

        for (auto* child : children) {
            child->_SerializeInto(pred, out);
        }
    }
};

template <bool IsNestable>
struct Node : public NodeBase {
  public:
    bool Nestable() const override { return IsNestable; }

    virtual void AppendChild(NodeBase* child, uint32_t z = std::numeric_limits<uint32_t>::max()) override {
        if constexpr (!IsNestable) {
            return;
        }

        if (!child || child == this) {
            return;
        }

        if (child->parent == this) {
            if (z == std::numeric_limits<uint32_t>::max()) {
                return;
            }

            child->SetZIndex(z);
            Sort(false);
            return;
        }

        if (child->parent) {
            child->parent->RemoveChild(child);
        }

        if (z == std::numeric_limits<uint32_t>::max()) {
            if (children.empty()) {
                z = 0;
            }

            else {
                z = children.back()->GetZIndex() + 1;
            }
        }

        child->SetZIndex(z);

        children.push_back(child);
        child->parent = this;

        Sort(false);
    }

    virtual void RemoveChild(NodeBase* child) override {
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

    void Sort(bool recursive = true) override {
        if constexpr (!IsNestable) {
            return;
        }

        std::stable_sort(children.begin(), children.end(), [](NodeBase* lhs, NodeBase* rhs) {
            return lhs->GetZIndex() < rhs->GetZIndex();
        });

        RefreshChildrenLinks();

        if (recursive) {
            for (auto* child : children) {
                child->Sort(true);
            }
        }
    }
};

template <bool IsNestable = false>
class RenderableNode : public Node<IsNestable>, public RenderableObject {
  public:
    virtual RenderableObject* HitTest(const Gdiplus::Point& screenPos) override {
        if (!BaseHitTest(screenPos)) {
            return nullptr;
        }

        if constexpr (IsNestable) {
            const std::vector<NodeBase*>& children     = this->GetChildren();
            RenderableObject*             targetObject = nullptr;

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

    bool PreRender(Foundation::Managers::Renderer& renderer) override {
        if (!IsVisible()) {
            return false;
        }

        auto& graphics   = renderer.AllocGraphics();
        auto  targetRect = GetLayoutRect();

        const auto state = graphics.Save();
        graphics.TranslateTransform(targetRect.X * 1.f, targetRect.Y * 1.f);
        graphics.IntersectClip(Gdiplus::Rect(0, 0, targetRect.Width, targetRect.Height));

        Render(renderer);

        if constexpr (IsNestable) {
            for (auto* child : this->GetChildren()) {
                auto node = static_cast<Node<IsNestable>*>(child);
                static_cast<RenderableNode*>(node)->PreRender(renderer);
            }
        }

        graphics.Restore(state);
        return true;
    }
};
} // namespace Lyra::UI::Foundation::Base