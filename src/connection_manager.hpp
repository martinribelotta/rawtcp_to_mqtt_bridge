#ifndef TCP_MQTT_BRIDGE_CONNECTION_MANAGER_HPP
#define TCP_MQTT_BRIDGE_CONNECTION_MANAGER_HPP

#include "tcp_context.hpp"
#include "slip.hpp"
#include "packet_parser.hpp"
#include "packet_processor.hpp"
#include "mqtt_client.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string>

class ConnectionManager {
public:
    explicit ConnectionManager(boost::asio::ip::tcp::socket& socket, const PacketDb& packet_db, MqttClient& mqtt_client);
    ~ConnectionManager() = default;

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    ConnectionManager(ConnectionManager&&) noexcept = default;
    ConnectionManager& operator=(ConnectionManager&&) noexcept = default;

    void handlePacket(std::span<const uint8_t> packet);
    void handleData(std::span<const uint8_t> data);
    void reset();

    const std::string& address() const { return address_; }

private:
    void sendResponse(const std::vector<uint8_t>& response);

    boost::asio::ip::tcp::socket& socket_;
    std::string address_;
    PacketProcessor packet_processor_;
    slip::Decoder decoder_;
    MqttClient& mqtt_client_;
};

#endif // TCP_MQTT_BRIDGE_CONNECTION_MANAGER_HPP
