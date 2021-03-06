#ifndef PARTICLEEMITTER_H_INCLUDED
#define PARTICLEEMITTER_H_INCLUDED

// Onut
#include <onut/Maths.h>

// Forward
#include <onut/ForwardDeclaration.h>
OForwardDeclare(ParticleEmitterDesc);
OForwardDeclare(ParticleSystemManager);

namespace onut
{
    class Particle;

    class ParticleEmitter final
    {
    public:
        ParticleEmitter(const OParticleEmitterDescRef& pEmitterDesc, const OParticleSystemManagerRef& pParticleSystemManager, const Matrix& transform, uint32_t instanceId = 0);
        ~ParticleEmitter();

        bool isAlive() const { return m_isAlive; }
        void stop();

        void update();
        void render();

        void setTransform(const Matrix& transform);
        uint32_t getInstanceId() const { return m_instanceId; }

        void setRenderEnabled(bool renderEnabled);
        bool getRenderEnabled() const { return m_renderEnabled; }

        Vector3 getPosition() const { return m_transform.Translation(); }
        const OParticleEmitterDescRef& getDesc() const { return m_pDesc; }

    private:
        Particle* spawnParticle();

        std::vector<Particle*> m_particles;
        OParticleSystemManagerRef m_pParticleSystemManager;
        bool m_isAlive = false;
        Matrix m_transform;
        OParticleEmitterDescRef m_pDesc;
        float m_rateProgress = 0.f;
        uint32_t m_instanceId = 0;
        bool m_isStopped = false;
        bool m_renderEnabled = true;
        float m_duration = 0.f;
    };
}

#endif
