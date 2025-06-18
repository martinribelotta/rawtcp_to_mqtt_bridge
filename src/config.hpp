#ifndef TCP_MQTT_BRIDGE_CONFIG_HPP
#define TCP_MQTT_BRIDGE_CONFIG_HPP

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <unordered_map>

class Configuration {
public:
    struct TcpConfig {
        unsigned short port = 12345;
        std::string bind_address = "0.0.0.0";
    };

    struct MqttConfig {
        std::string host = "localhost";
        uint16_t port = 1883;
        std::string client_id = "tcp_bridge";

        std::string getBrokerUrl() const {
            return fmt::format("tcp://{}:{}", host, port);
        }
    };

    TcpConfig tcp;
    MqttConfig mqtt;
    struct PacketDefsConfig {
        std::vector<std::string> paths;
        std::vector<std::string> patterns = {"*.yaml", "*.yml"};
    };

    std::string log_level = "debug";
    PacketDefsConfig packet_defs;

    static Configuration fromYaml(const std::string& path);
    static spdlog::level::level_enum parseLogLevel(const std::string& level);
};

#endif // TCP_MQTT_BRIDGE_CONFIG_HPP
