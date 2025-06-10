#ifndef TCP_MQTT_BRIDGE_MQTT_CLIENT_HPP
#define TCP_MQTT_BRIDGE_MQTT_CLIENT_HPP

#include "config.hpp"
#include <boost/asio.hpp>
#include <boost/mqtt5.hpp>
#include <memory>
#include <string>
#include <mutex>
#include <thread>

class MqttClient {
public:
    explicit MqttClient(boost::asio::io_context& ioc, const Configuration::MqttConfig& config);
    ~MqttClient();

    // Connect to the MQTT broker
    void connect();

    // Publish a message to a topic
    void publish(const std::string& topic, const std::string& payload, uint8_t qos = 1);

    // Stop the MQTT client
    void stop();

    // Get MQTT configuration
    const Configuration::MqttConfig& getConfig() const { return config_; }

private:
    void setup_client();
    void handle_close();
    void handle_error(boost::system::error_code const& ec);

    boost::asio::io_context& io_ctx_;
    const Configuration::MqttConfig& config_;

    using client_t =  boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket, std::monostate, boost::mqtt5::logger>;

    std::shared_ptr<client_t> client_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> stopped_{false};
};

#endif // TCP_MQTT_BRIDGE_MQTT_CLIENT_HPP
