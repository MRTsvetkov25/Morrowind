#ifndef OPENMW_PACKETSCRIPTGLOBALSHORT_HPP
#define OPENMW_PACKETSCRIPTGLOBALSHORT_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketScriptGlobalShort : public WorldPacket
    {
    public:
        PacketScriptGlobalShort(RakNet::RakPeerInterface *peer);

        virtual void Object(WorldObject &worldObject, bool send);
    };
}

#endif //OPENMW_PACKETSCRIPTGLOBALSHORT_HPP
