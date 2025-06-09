#include "connection_manager.hpp"
#include "packet_parser.hpp"
#include "packet_handler.hpp"
#include <spdlog/spdlog.h>

ConnectionManager::ConnectionManager(boost::asio::ip::tcp::socket& socket, const PacketDb& packet_db)
    : socket_(socket)
    , address_(socket.remote_endpoint().address().to_string())
    , packet_processor_(packet_db)
{
    decoder_.setPacketHandler([this](std::span<const uint8_t> packet) {
        this->handlePacket(packet);
    });
    packet_processor_.setHandler(std::shared_ptr<PacketHandler>(this, [](PacketHandler*) {})); // No delete this
}

void ConnectionManager::handlePacket(std::span<const uint8_t> packet) {
    spdlog::debug("Decoded packet of {} bytes from {}", packet.size(), address_);
    packet_processor_.processPacket(packet);
}

void ConnectionManager::onPacketField(const FieldView& field) {
    // TODO render packet on template, publish it on MQTT
    spdlog::debug("Field {} received in packet", field.desc.name);
}

void ConnectionManager::onPacketComplete(std::span<const uint8_t> rawPacket) {
    sendResponse(slip::Decoder::makeResponse(slip::ACK));
}

void ConnectionManager::handleData(std::span<const uint8_t> data) {
    spdlog::debug("Raw data {} bytes from {}", data.size(), address_);
    try {
        decoder_.decode(data);
    } catch (const slip::SlipError& e) {
        spdlog::error("SLIP decode error from {}: {}", address_, e.what());
        reset();
    }
}

void ConnectionManager::sendResponse(const std::vector<uint8_t>& response) {
    boost::asio::async_write(socket_, boost::asio::buffer(response),
        [addr = address_](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (ec) {
                spdlog::error("Error sending packet to {}: {}", addr, ec.message());
            } else {
                spdlog::debug("Sent SLIP packet {} bytes to {}", bytes_transferred, addr);
            }
        });
}

void ConnectionManager::reset() {
    decoder_.reset();
}
