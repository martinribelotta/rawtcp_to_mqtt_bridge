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
            config.mqtt.broker = mqtt["broker"].as<std::string>();
            config.mqtt.client_id = mqtt["client_id"].as<std::string>();
            if (const auto& topics = mqtt["topics"]) {
                config.mqtt.topics = topics.as<std::vector<std::string>>();
            }
            if (mqtt["topic_template"]) {
                config.mqtt.topic_template = mqtt["topic_template"].as<std::string>();
            }
            if (mqtt["payload_template"]) {
                config.mqtt.payload_template = mqtt["payload_template"].as<std::string>();
            }
        }
        if (const auto& logging = yaml["logging"]) {
            config.log_level = logging["level"].as<std::string>("debug");
        }
        if (yaml["packet_def"]) {
            config.packet_def_path = yaml["packet_def"].as<std::string>();
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
