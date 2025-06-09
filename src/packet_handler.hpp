#ifndef TCP_MQTT_BRIDGE_PACKET_HANDLER_HPP
#define TCP_MQTT_BRIDGE_PACKET_HANDLER_HPP

#include "packet_parser.hpp"
#include <span>

class PacketHandler {
public:
    virtual ~PacketHandler() = default;
    
    virtual void onPacketField(const FieldView& field) = 0;
    
    virtual void onPacketComplete(std::span<const uint8_t> rawPacket) {}
};

#endif // TCP_MQTT_BRIDGE_PACKET_HANDLER_HPP
