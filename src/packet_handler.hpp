#ifndef TCP_MQTT_BRIDGE_PACKET_HANDLER_HPP
#define TCP_MQTT_BRIDGE_PACKET_HANDLER_HPP

#include "packet_parser.hpp"
#include <span>

// Interfaz para el manejo de paquetes
class PacketHandler {
public:
    virtual ~PacketHandler() = default;
    
    // Llamado cuando se recibe un campo de un paquete
    virtual void onPacketField(const FieldView& field) = 0;
    
    // Llamado cuando se completa un paquete (opcional)
    virtual void onPacketComplete(std::span<const uint8_t> rawPacket) {}
};

#endif // TCP_MQTT_BRIDGE_PACKET_HANDLER_HPP
