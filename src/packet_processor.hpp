#ifndef TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP
#define TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP

#include "packet_parser.hpp"
#include "packet_handler.hpp"
#include <memory>
#include <span>

class PacketProcessor {
public:
    explicit PacketProcessor(const PacketDb& packet_db);

    // Establece el manejador de paquetes
    void setHandler(std::shared_ptr<PacketHandler> handler) { handler_ = std::move(handler); }

    // Procesa un paquete y notifica al handler
    std::pair<size_t, size_t> processPacket(std::span<const uint8_t> packet);

private:
    const PacketDb& packet_db_;
    std::shared_ptr<PacketHandler> handler_;
};

#endif // TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP
