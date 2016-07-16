#pragma once

// Onut includes
#include <onut/Component.h>
#include <onut/Maths.h>

// Forward declarations
#include <onut/ForwardDeclaration.h>
OForwardDeclare(Collider2DComponent);
class b2Body;

namespace onut
{
    class Collider2DComponent final : public Component
    {
    public:
        ~Collider2DComponent();

        const Vector2& getSize() const;
        void setSize(const Vector2& size);

        bool getTrigger() const;
        void setTrigger(bool trigger);

        float getPhysicScale() const;
        void setPhysicScale(float scale);

        const Vector2& getVelocity() const;
        void setVelocity(const Vector2& velocity);

    private:
        void onCreate() override;
        void onUpdate() override;

        bool m_trigger = false;
        Vector2 m_size = Vector2::One;
        float m_physicScale = 1.0f;
        b2Body* m_pBody;
        Vector2 m_velocity;
    };
};
