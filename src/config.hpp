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
        std::string broker = "tcp://localhost:1883";
        std::string client_id = "tcp_bridge";
        std::vector<std::string> topics;
    };

    TcpConfig tcp;
    MqttConfig mqtt;
    std::string log_level = "debug";

    static Configuration fromYaml(const std::string& path);
    static spdlog::level::level_enum parseLogLevel(const std::string& level);
};

#endif // TCP_MQTT_BRIDGE_CONFIG_HPP
