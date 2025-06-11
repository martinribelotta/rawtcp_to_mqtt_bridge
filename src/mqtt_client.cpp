#include "mqtt_client.hpp"

#include <spdlog/spdlog.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

MqttClient::MqttClient(boost::asio::io_context& ioc, const Configuration::MqttConfig& config)
    : config_(config)
    , client_(ioc, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::error))
{
    setup_client();
}

MqttClient::~MqttClient()
{
    stop();
}

void MqttClient::setup_client()
{
    client_.brokers(config_.host, config_.port);
}

void MqttClient::connect()
{
    spdlog::info("Connecting to MQTT broker at {}:{}", config_.host, config_.port);
    client_.async_run(
        [this](boost::system::error_code ec) {
            if (ec) {
                spdlog::error("Failed to connect to MQTT broker: {}", ec.message());
                handle_error(ec);
            } else {
                spdlog::info("Connected to MQTT broker at {}:{}", config_.host, config_.port);
            }
        }
    );
}

void MqttClient::publish(const std::string& topic, const std::string& payload, uint8_t qos)
{
    client_.async_publish<boost::mqtt5::qos_e::at_most_once>(
        topic,
        payload,
        boost::mqtt5::retain_e::yes, boost::mqtt5::publish_props {},
        [this, topic, payload](boost::system::error_code ec) {
            if (ec) {
                spdlog::error("Failed to publish message to topic '{}': {}", topic, ec.message());
                handle_error(ec);
            } else {
                spdlog::debug("Published message to topic '{}': {}", topic, payload);
            }
        }
    );
}

void MqttClient::stop() {
    client_.async_disconnect(
        [this](boost::system::error_code ec) {
            if (ec) {
                spdlog::error("Error during MQTT client disconnect: {}", ec.message());
            } else {
                spdlog::info("MQTT client disconnected successfully.");
            }
            handle_close();
        }
    );
}

void MqttClient::handle_error(boost::system::error_code const& ec)
{
    if (ec) {
        spdlog::error("MQTT client error: {}", ec.message());
    } else {
        spdlog::info("MQTT client operation completed successfully.");
    }
    
    // Handle specific error cases if needed
    if (ec == boost::asio::error::operation_aborted) {
        spdlog::warn("MQTT client operation was aborted.");
    } else if (ec == boost::asio::error::connection_reset) {
        spdlog::warn("MQTT connection was reset by the peer.");
    }
}

void MqttClient::handle_close()
{
    spdlog::info("MQTT client has been stopped.");
}
