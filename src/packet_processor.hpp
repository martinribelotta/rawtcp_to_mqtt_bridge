#ifndef TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP
#define TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP

#include "packet_parser.hpp"
#include <memory>
#include <span>

#include "inja/inja.hpp"

class PacketProcessor {
public:
    using json_t = nlohmann::json;

    explicit PacketProcessor(const PacketDb& packet_db);

    // Procesa un paquete y notifica al handler
    std::pair<size_t, size_t> processPacket(std::span<const uint8_t> packet);

    json_t& getJsonDb() { return json_db; }
    const json_t& getJsonDb() const { return json_db; }

private:
    json_t json_db;
    const PacketDb& packet_db_;
};

#endif // TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP
