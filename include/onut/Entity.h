#pragma once
// Onut includes
#include <onut/Maths.h>

// STL
#include <vector>

// Forward declarations
#include <onut/ForwardDeclaration.h>
OForwardDeclare(Component);
OForwardDeclare(Entity);
OForwardDeclare(EntityManager);

namespace onut
{
    class Entity final : public std::enable_shared_from_this<Entity>
    {
    public:
        static OEntityRef create(const OEntityManagerRef& pEntityManager = nullptr);

        ~Entity();

        void destroy();
        OEntityRef copy() const;

        const Matrix& getLocalTransform() const;
        const Matrix& getWorldTransform();
        void setLocalTransform(const Matrix& localTransform);
        void setWorldTransform(const Matrix& worldTransform);

        void add(const OEntityRef& pChild);
        void remove(const OEntityRef& pChild);
        void remove();
        OEntityRef getParent() const;

        bool isEnabled() const;
        void setEnabled(bool isEnabled);
        bool isVisible() const;
        void setVisible(bool isVisible);
        bool isStatic() const;
        void setStatic(bool isStatic);

        template<typename Tcomponent>
        std::shared_ptr<Tcomponent> getComponent() const
        {
            for (auto& pComponent : m_components)
            {
                if (dynamic_cast<Tcomponent*>(pComponent.get()))
                {
                    return ODynamicCast<Tcomponent>(pComponent);
                }
            }
            return nullptr;
        }

        template<typename Tcomponent>
        std::shared_ptr<Tcomponent> addComponent()
        {
            auto pComponent = getComponent<Tcomponent>();
            if (pComponent) return pComponent;
            pComponent = std::shared_ptr<Tcomponent>(new Tcomponent());
            addComponent(pComponent);
            return pComponent;
        }

    private:
        friend class Component;
        friend class EntityManager;

        using Entities = std::vector<OEntityRef>;
        using Components = std::vector<OComponentRef>;

        Entity();

        void dirtyWorld();
        void addComponent(const OComponentRef& pComponent);
        void render2d();

        bool m_isWorldDirty = true;
        Matrix m_localTransform;
        Matrix m_worldTransform;
        OEntityWeak m_pParent;
        Components m_components;
        Entities m_children;
        OEntityManagerRef m_pEntityManager;
        bool m_isEnabled = true;
        bool m_isVisible = true;
        bool m_isStatic = false;
    };
};