#include "connection_manager.hpp"
#include "packet_parser.hpp"
#include <spdlog/spdlog.h>
#include <inja/inja.hpp>

ConnectionManager::ConnectionManager(boost::asio::ip::tcp::socket& socket, const PacketDb& packet_db, MqttClient& mqtt_client)
    : socket_(socket)
    , address_(socket.remote_endpoint().address().to_string())
    , packet_processor_(packet_db)
    , mqtt_client_(mqtt_client)
    , config_(mqtt_client.getConfig())
{
    decoder_.setPacketHandler([this](std::span<const uint8_t> packet) {
        this->handlePacket(packet);
    });
}

void ConnectionManager::handlePacket(std::span<const uint8_t> packet) {
    spdlog::debug("Decoded packet of {} bytes from {}", packet.size(), address_);
    auto [packet_size, fields_count] = packet_processor_.processPacket(packet);
    
    // Get the collected JSON data
    const auto& json_data = packet_processor_.getJsonDb();
    
    // Create inja environment and render templates
    inja::Environment env;
    
    try {
        // Render topic using the template
        std::string rendered_topic = env.render(config_.topic_template, json_data);
        // Render payload using the template
        std::string rendered_payload = env.render(config_.payload_template, json_data);
        
        // Publish to MQTT
        mqtt_client_.publish(rendered_topic, rendered_payload);
        
        spdlog::debug("Published MQTT message - Topic: {}, Payload: {}", rendered_topic, rendered_payload);
    } catch (const std::exception& e) {
        spdlog::error("Error rendering MQTT templates: {}", e.what());
    }
    
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
