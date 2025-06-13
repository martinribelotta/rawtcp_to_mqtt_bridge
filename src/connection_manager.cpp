#include "connection_manager.hpp"
#include "packet_parser.hpp"
#include <spdlog/spdlog.h>
#include <inja/inja.hpp>

ConnectionManager::ConnectionManager(boost::asio::ip::tcp::socket& socket, const PacketDb& packet_db, MqttClient& mqtt_client)
    : socket_(socket)
    , address_(socket.remote_endpoint().address().to_string())
    , packet_processor_(packet_db, mqtt_client)
    , mqtt_client_(mqtt_client)
{
    decoder_.setPacketHandler([this](std::span<const uint8_t> packet) {
        this->handlePacket(packet);
    });
}

void ConnectionManager::handlePacket(std::span<const uint8_t> packet) {
    spdlog::debug("Decoded packet of {} bytes from {}", packet.size(), address_);
    
    if (auto mqtt_message = packet_processor_.processPacket(packet)) {
        mqtt_client_.publish(
            mqtt_message->topic,
            mqtt_message->payload,
            [this](boost::system::error_code ec) {
                if (ec) {
                    spdlog::error("Failed to publish MQTT message: {}", ec.message());
                    sendResponse(slip::Decoder::makeResponse(slip::NAK));
                } else {
                    spdlog::debug("MQTT message published successfully");
                    sendResponse(slip::Decoder::makeResponse(slip::ACK));
                }
            },
            mqtt_message->qos,
            mqtt_message->retain
        );
    }
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
