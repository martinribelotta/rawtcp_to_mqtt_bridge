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

void MqttClient::publish(const std::string& topic, const std::string& payload, PublishCallback callback, uint8_t qos, bool retain)
{
    auto retain_flag = retain ? boost::mqtt5::retain_e::yes : boost::mqtt5::retain_e::no;
    boost::mqtt5::publish_props props;

    switch (qos) {
        case 0:
            client_.async_publish<boost::mqtt5::qos_e::at_most_once>(
                topic, payload,
                retain_flag, props,
                [this, callback = std::move(callback)](boost::system::error_code ec) {
                    handle_error(ec);
                    callback(ec);
                }
            );
            break;
        case 1:
            client_.async_publish<boost::mqtt5::qos_e::at_least_once>(
                topic, payload,
                retain_flag, props,
                [this, callback = std::move(callback)](boost::system::error_code ec, boost::mqtt5::reason_code rc, boost::mqtt5::puback_props) {
                    handle_error(ec);
                    callback(ec);
                }
            );
            break;
        case 2:
            client_.async_publish<boost::mqtt5::qos_e::exactly_once>(
                topic, payload,
                retain_flag, props,
                [this, callback = std::move(callback)](boost::system::error_code ec, boost::mqtt5::reason_code rc, boost::mqtt5::pubcomp_props) {
                    handle_error(ec);
                    callback(ec);
                }
            );
            break;
        default:
            spdlog::error("Invalid QoS value: {}. Must be 0, 1, or 2.", qos);
            callback(boost::asio::error::invalid_argument);
            break;
    }
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
