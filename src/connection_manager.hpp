#ifndef TCP_MQTT_BRIDGE_CONNECTION_MANAGER_HPP
#define TCP_MQTT_BRIDGE_CONNECTION_MANAGER_HPP

#include "tcp_context.hpp"
#include "slip.hpp"
#include "packet_parser.hpp"
#include "packet_handler.hpp"
#include "packet_processor.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string>

class ConnectionManager : public PacketHandler {
public:
    explicit ConnectionManager(boost::asio::ip::tcp::socket& socket, const PacketDb& packet_db);
    ~ConnectionManager() = default;

    // Prevent copying
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // Allow moving
    ConnectionManager(ConnectionManager&&) noexcept = default;
    ConnectionManager& operator=(ConnectionManager&&) noexcept = default;

    // Interfaz PacketHandler
    void onPacketField(const FieldView& field) override;
    void onPacketComplete(std::span<const uint8_t> rawPacket) override;

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
};

#endif // TCP_MQTT_BRIDGE_CONNECTION_MANAGER_HPP
