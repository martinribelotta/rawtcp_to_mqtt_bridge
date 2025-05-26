#ifndef TCP_MQTT_BRIDGE_SERVER_MANAGER_HPP
#define TCP_MQTT_BRIDGE_SERVER_MANAGER_HPP

#include "config.hpp"
#include "tcp_server.hpp"
#include "slip.hpp"
#include <boost/asio.hpp>

class ServerManager {
public:
    explicit ServerManager(const Configuration& config);
    ~ServerManager();

    // Prevent copying
    ServerManager(const ServerManager&) = delete;
    ServerManager& operator=(const ServerManager&) = delete;

    void run();
    void stop();

private:
    void setupEventHandlers();

    boost::asio::io_context io_ctx_;
    TcpServer server_;
    const Configuration& config_;
    bool stopped_{false};
};

#endif // TCP_MQTT_BRIDGE_SERVER_MANAGER_HPP
