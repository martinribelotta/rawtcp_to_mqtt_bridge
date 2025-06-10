#include "packet_processor.hpp"

#include <spdlog/spdlog.h>

PacketProcessor::PacketProcessor(const PacketDb& packet_db)
    : packet_db_(packet_db)
{
}

std::pair<size_t, size_t> PacketProcessor::processPacket(std::span<const uint8_t> packet)
{
    json_db.clear();

    auto result = scan_packets(packet_db_, packet, 
        [this](const FieldView& field) {
            // TODO render here
            auto name = field.desc.name;
            auto value = field.value.to_string();
            json_db[name] = value;
            spdlog::debug("Field: {} = {}", name, value);
        });
    return result;
}
