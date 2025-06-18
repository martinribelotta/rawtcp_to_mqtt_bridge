#include "config.hpp"

Configuration Configuration::fromYaml(const std::string& path) {
    Configuration config;
    try {
        auto yaml = YAML::LoadFile(path);
        if (const auto& tcp = yaml["tcp"]) {
            config.tcp.port = tcp["port"].as<unsigned short>();
            config.tcp.bind_address = tcp["bind"].as<std::string>();
        }
        if (const auto& mqtt = yaml["mqtt"]) {
            if (mqtt["broker"]) {
                std::string broker = mqtt["broker"].as<std::string>();
                if (broker.substr(0, 6) == "tcp://") {
                    broker = broker.substr(6);
                }
                auto colon_pos = broker.find(':');
                if (colon_pos != std::string::npos) {
                    config.mqtt.host = broker.substr(0, colon_pos);
                    config.mqtt.port = std::stoi(broker.substr(colon_pos + 1));
                } else {
                    config.mqtt.host = broker;
                    config.mqtt.port = 1883;
                }
            } else {
                config.mqtt.host = mqtt["host"].as<std::string>("localhost");
                config.mqtt.port = mqtt["port"].as<uint16_t>(1883);
            }
            config.mqtt.client_id = mqtt["client_id"].as<std::string>();
        }
        if (const auto& logging = yaml["logging"]) {
            config.log_level = logging["level"].as<std::string>("debug");
        }
        if (const auto& packet_defs = yaml["packet_defs"]) {
            if (const auto& paths = packet_defs["paths"]) {
                config.packet_defs.paths = paths.as<std::vector<std::string>>();
            }
            if (const auto& patterns = packet_defs["patterns"]) {
                config.packet_defs.patterns = patterns.as<std::vector<std::string>>();
            }
        }
    } catch (const YAML::Exception& e) {
        spdlog::warn("Config parse error: {}. Using defaults.", e.what());
    }
    return config;
}

spdlog::level::level_enum Configuration::parseLogLevel(const std::string& level) {
    static const std::unordered_map<std::string, spdlog::level::level_enum> levels = {
        {"trace", spdlog::level::trace},
        {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},
        {"critical", spdlog::level::critical},
        {"off", spdlog::level::off}
    };
    auto it = levels.find(level);
    return it != levels.end() ? it->second : spdlog::level::debug;
}
