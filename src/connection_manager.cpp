#include "connection_manager.hpp"
#include <spdlog/spdlog.h>

ConnectionManager::ConnectionManager(boost::asio::ip::tcp::socket& socket)
    : socket_(socket)
    , address_(socket.remote_endpoint().address().to_string())
{
    decoder_.setPacketHandler([this](std::span<const uint8_t> packet) {
        this->handlePacket(packet);
    });
}

void ConnectionManager::handlePacket(std::span<const uint8_t> packet) {
    spdlog::debug("Decoded packet of {} bytes from {}", packet.size(), address_);
    // TODO implement packet decoding logic here
    // For now, just send an ACK response
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
