#include <TiltedOnlinePCH.h>
#include <NetImmerse/NiAVObject.h>

void NiAVObject::SetCollisionLayer(uint32_t aCollisionLayer) noexcept
{
    using TSetCollisionLayer = void(NiAVObject*, uint32_t);
    POINTER_SKYRIMSE(TSetCollisionLayer, s_setCollisionLayer, 76170);
    s_setCollisionLayer.Get()(this, aCollisionLayer);
}
