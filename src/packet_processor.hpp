#ifndef TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP
#define TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP

#include "packet_parser.hpp"
#include "mqtt_client.hpp"

#include <memory>
#include <span>

#include "inja/inja.hpp"

class PacketProcessor {
public:
    using json_t = nlohmann::json;
    struct MqttMessage {
        std::string topic;
        std::string payload;
        uint8_t qos;
        bool retain;
    };

    PacketProcessor(const PacketDb& packet_db, MqttClient& mqtt_client);

    std::optional<MqttMessage> processPacket(std::span<const uint8_t> packet);

private:
    json_t json_db;
    const PacketDb& packet_db_;
    MqttClient& mqtt_client_;
};

#endif // TCP_MQTT_BRIDGE_PACKET_PROCESSOR_HPP
