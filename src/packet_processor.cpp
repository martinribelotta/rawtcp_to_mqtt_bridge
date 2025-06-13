#include "packet_processor.hpp"

#include <spdlog/spdlog.h>

PacketProcessor::PacketProcessor(const PacketDb& packet_db, MqttClient& mqtt_client)
    : packet_db_(packet_db)
    , mqtt_client_(mqtt_client)
{
}

std::optional<PacketProcessor::MqttMessage> PacketProcessor::processPacket(std::span<const uint8_t> packet)
{
    json_db.clear();
    const PacketDesc* current_packet = nullptr;

    auto result = scan_packets(packet_db_, packet, 
        [this, &current_packet](const FieldView& field, const PacketDesc& packet) {
            if (!current_packet) current_packet = &packet;
            auto name = field.desc.name;
            auto value = field.value.to_string();
            json_db[name] = value;
            spdlog::debug("Field: {} = {}", name, value);
        });
    
    if (!current_packet) {
        spdlog::error("No packet matched the input data");
        return std::nullopt;
    }

    try {
        inja::Environment env;
        std::string rendered_topic = env.render(current_packet->mqtt.topic, json_db);
        std::string rendered_payload = env.render(current_packet->mqtt.payload, json_db);
        return MqttMessage{
            rendered_topic,
            rendered_payload,
            current_packet->mqtt.qos,
            current_packet->mqtt.retain
        };
    } catch (const std::exception& e) {
        spdlog::error("Error rendering MQTT templates: {}", e.what());
        return std::nullopt;
    }
}
