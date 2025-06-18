#ifndef TCP_MQTT_BRIDGE_SERVER_MANAGER_HPP
#define TCP_MQTT_BRIDGE_SERVER_MANAGER_HPP

#include "config.hpp"
#include "tcp_server.hpp"
#include "packet_parser.hpp"
#include "packet_parser_yaml.hpp"
#include "slip.hpp"
#include "mqtt_client.hpp"
#include <boost/asio.hpp>

class ServerManager {
public:
    explicit ServerManager(const Configuration& config, const PacketDb& packet_db);
    ~ServerManager();

    ServerManager(const ServerManager&) = delete;
    ServerManager& operator=(const ServerManager&) = delete;

    void run();
    void stop();

private:
    void setupEventHandlers();

    boost::asio::io_context io_ctx_;
    TcpServer server_;
    std::unique_ptr<MqttClient> mqtt_client_;
    const Configuration& config_;
    const PacketDb& packet_db_;
    bool stopped_{false};
};

#endif // TCP_MQTT_BRIDGE_SERVER_MANAGER_HPP
