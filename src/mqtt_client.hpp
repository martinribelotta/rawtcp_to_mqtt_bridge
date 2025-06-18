#ifndef TCP_MQTT_BRIDGE_MQTT_CLIENT_HPP
#define TCP_MQTT_BRIDGE_MQTT_CLIENT_HPP

#include "config.hpp"

#include <boost/asio.hpp>
#include <boost/mqtt5.hpp>
#include <string>

class MqttClient {
public:
    explicit MqttClient(boost::asio::io_context& ioc, const Configuration::MqttConfig& config);
    ~MqttClient();

    void connect();
    using PublishCallback = std::function<void(boost::system::error_code)>;
    void publish(const std::string& topic, const std::string& payload, PublishCallback callback, uint8_t qos = 1, bool retain = false);

    void stop();

    const Configuration::MqttConfig& getConfig() const { return config_; }

private:
    void setup_client();
    void handle_close();
    void handle_error(boost::system::error_code const& ec);

    const Configuration::MqttConfig& config_;

    using client_t =  boost::mqtt5::mqtt_client<
            boost::asio::ip::tcp::socket,
            std::monostate,
            boost::mqtt5::logger>;

    client_t client_;
};

#endif // TCP_MQTT_BRIDGE_MQTT_CLIENT_HPP
