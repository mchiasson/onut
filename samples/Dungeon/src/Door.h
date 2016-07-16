#pragma once
#include <onut/Component.h>
#include <onut/Point.h>
#include <onut/TiledMap.h>

// Forware declarations
#include <onut/ForwardDeclaration.h>
OForwardDeclare(Collider2DComponent);
OForwardDeclare(Sound);
OForwardDeclare(TiledMapComponent);
ForwardDeclare(Door);

class Door final : public OComponent
{
public:
    bool getOpen() const;
    void setOpen(bool open);

    OSoundRef getOpenSound() const;
    void setOpenSound(const OSoundRef& pSound);

    OSoundRef getCloseSound() const;
    void setCloseSound(const OSoundRef& pSound);

    OEntityRef getTarget() const;
    void setTarget(const OEntityRef& pTarget);

    const Point& getMapPos() const;

protected:
    void onCreate() override;

private:
    Point m_mapPos;
    bool m_open = false;
    Vector2 m_openSize;
    Vector2 m_closeSize;
    OCollider2DComponentRef m_pCollider;
    OTiledMapRef m_pTiledMap;
    OTiledMapComponentRef m_pTiledMapComponent;
    OTiledMap::TileLayer* m_pTileLayer;
    OSoundRef m_pOpenSound;
    OSoundRef m_pCloseSound;
    OEntityWeak m_pTargetDoor;
};