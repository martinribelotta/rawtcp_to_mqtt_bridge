#include "packet_processor.hpp"

PacketProcessor::PacketProcessor(const PacketDb& packet_db)
    : packet_db_(packet_db)
{
}

std::pair<size_t, size_t> PacketProcessor::processPacket(std::span<const uint8_t> packet)
{
    if (!handler_) {
        return {0, 0};
    }

    auto result = scan_packets(packet_db_, packet, 
        [this](const FieldView& field) { handler_->onPacketField(field); });
    
    if (result.first > 0) {
        handler_->onPacketComplete(packet);
    }
    
    return result;
}
