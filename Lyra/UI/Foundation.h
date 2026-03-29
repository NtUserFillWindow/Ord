#pragma once
#include <random>
#include <vector>

#include <gsl/gsl>

#include "Native.h"

// forward declaration
namespace Lyra::UI::Components {
class Window;
};

namespace Lyra::UI::Foundation {
class Renderer final {
  public:
    Renderer()                           = default;
    Renderer(Renderer&&)                 = delete;
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(Renderer&&)      = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer()                          = default;

    Gdiplus::Graphics& AllocGraphics() {
        if (_graphics == nullptr) {
            _graphics = new Gdiplus::Graphics{_swapchain.GetSwapchainContext()};
        }

        return *_graphics;
    };

    void Invalidate(const Gdiplus::Rect invalidatedRect) const {
        RECT rect{};
        rect.left   = invalidatedRect.X;
        rect.top    = invalidatedRect.Y;
        rect.right  = invalidatedRect.GetRight();
        rect.bottom = invalidatedRect.GetBottom();
        InvalidateRect(_swapchain.GetTargetWindow(), &rect, FALSE);
    }

  private:
    friend class Components::Window;

    void ApplyWindow(HWND windowHandle) { _swapchain.BindToWindow(windowHandle); };

    bool Present() { return _swapchain.PresentBuffer(); };
    void UpdateSize(LPARAM lParam) {
        _swapchain.UpdateSize(lParam);

        if (_swapchain.Invalid()) {
            return;
        }

        if (_graphics != nullptr) {
            delete _graphics;
        }

        _graphics = new Gdiplus::Graphics{_swapchain.GetSwapchainContext()};
    };

  private:
    Native::Swapchain              _swapchain = {};
    gsl::owner<Gdiplus::Graphics*> _graphics  = nullptr;
};

class Object {
  public:
    std::wstring_view Type = L"Object";

    void Event();
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
    virtual bool Render(Foundation::Renderer& renderer) {
        auto& graphics = renderer.AllocGraphics();

        static std::random_device              randomDevice{};
        static std::mt19937                    randomEngine{randomDevice()};
        static std::uniform_int_distribution<> distribution{0x00, 0xFF};

        graphics.Clear(
            Gdiplus::Color(distribution(randomEngine), distribution(randomEngine), distribution(randomEngine))
        );

        return true;
    };
    virtual bool PreRender(Foundation::Renderer& renderer) = 0;

  public:
    auto IsVisible() const { return _isVisible; }
    void SetVisible(bool visible) { _isVisible = visible; }

    auto GetObjectRect() const { return _objectRect; }
    void SetObjectRect(Gdiplus::Rect rect) { _objectRect = rect; }

  private:
    bool          _isVisible  = true;
    Gdiplus::Rect _objectRect = {};
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
    std::size_t            zIndex   = 0;
    NodeBase*              parent   = nullptr;
    std::vector<NodeBase*> children = {};

    struct {
        NodeBase* prev = nullptr;
        NodeBase* next = nullptr;
    } sibling = {};

  public:
    virtual bool Nestable() const = 0;

    auto& GetChildren() { return children; }

    virtual void AppendChild(NodeBase* child, std::size_t z = std::numeric_limits<std::size_t>::max()) = 0;
    virtual void RemoveChild(NodeBase* child)                                                          = 0;
    virtual void Sort(bool recursive = true)                                                           = 0;

    std::size_t GetZIndex() const { return zIndex; }
    void        SetZIndex(std::size_t index) {
        zIndex = index;

        if (parent) {
            parent->Sort(false);
        }
    }

    void Reparent(NodeBase* newParent, std::size_t z = std::numeric_limits<std::size_t>::max()) {
        if (parent == newParent) {
            if (z != std::numeric_limits<std::size_t>::max()) {
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

        if (z != std::numeric_limits<std::size_t>::max()) {
            SetZIndex(z);
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
struct Node : NodeBase {
    bool Nestable() const override { return IsNestable; }

    void AppendChild(NodeBase* child, std::size_t z = std::numeric_limits<std::size_t>::max()) override {
        if constexpr (!IsNestable) {
            return;
        }

        if (!child || child == this) {
            return;
        }

        if (child->parent == this) {
            if (z == std::numeric_limits<std::size_t>::max()) {
                return;
            }

            child->SetZIndex(z);
            Sort(false);
            return;
        }

        if (child->parent) {
            child->parent->RemoveChild(child);
        }

        if (z == std::numeric_limits<std::size_t>::max()) {
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

    void RemoveChild(NodeBase* child) override {
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
    bool PreRender(Foundation::Renderer& renderer) override {
        if (!IsVisible()) {
            return false;
        }

        auto& graphics   = renderer.AllocGraphics();
        auto  targetRect = GetObjectRect();

        const auto state = graphics.Save();
        graphics.TranslateTransform(targetRect.X * 1.f, targetRect.Y * 1.f);
        graphics.IntersectClip(Gdiplus::Rect(0, 0, targetRect.Width, targetRect.Height));

        Render(renderer);

        if constexpr (IsNestable) {
            for (auto* child : this->GetChildren()) {
                ((RenderableNode*)(Node<IsNestable>*)child)->PreRender(renderer);
            }
        }

        graphics.Restore(state);
        return true;
    }
};
} // namespace Lyra::UI::Foundation