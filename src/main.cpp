#include "config.hpp"
#include "server_manager.hpp"
#include "packet_parser_yaml.hpp"
#include <boost/program_options.hpp>
#include <cpptrace/cpptrace.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    
    namespace po = boost::program_options;

    // Command line options
    po::options_description desc("TCP <-> MQTT Bridge Options");
    desc.add_options()
        ("help,h", "Show this help message")
        ("config,c", po::value<std::string>()->default_value("config.yaml"), "Configuration file path")
        ("port,p", po::value<unsigned short>(), "TCP port (overrides config)")
        ("bind,b", po::value<std::string>(), "Bind address (overrides config)")
        ("log-level,l", po::value<std::string>(), "Log level (trace,debug,info,warn,error,critical,off)")
        ("verbose,v", "Enable debug logging (shorthand)");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            desc.print(std::cout);
            return 0;
        }

        if (vm.count("verbose")) {
            spdlog::set_level(spdlog::level::debug);
            spdlog::debug("Verbose logging enabled");
        }
    } catch (const po::error& e) {
        spdlog::error("Command line error: {}", e.what());
        return 1;
    }

    std::string config_path = vm["config"].as<std::string>();
    // If path is relative, make it relative to current directory
    if (!config_path.empty() && config_path[0] != '/') {
        std::filesystem::path current = std::filesystem::current_path();
        std::filesystem::path config_file(config_path);
        config_path = (current / config_file).string();
    }
    auto config = Configuration::fromYaml(config_path);

    // Set log level from config, can be overridden by command line
    spdlog::set_level(Configuration::parseLogLevel(config.log_level));

    // Override with command line if specified
    if (vm.count("log-level")) {
        spdlog::set_level(Configuration::parseLogLevel(vm["log-level"].as<std::string>()));
    } else if (vm.count("verbose")) {
        spdlog::set_level(spdlog::level::debug);
    }

    // Override with command line if specified
    if (vm.count("port")) config.tcp.port = vm["port"].as<unsigned short>();
    if (vm.count("bind")) config.tcp.bind_address = vm["bind"].as<std::string>();

    // Resolver packet_def_path relativo al archivo de configuración si no es absoluto
    std::filesystem::path packet_def_path(config.packet_def_path);
    if (!config.packet_def_path.empty() && config.packet_def_path[0] != '/') {
        std::filesystem::path config_dir = std::filesystem::path(config_path).parent_path();
        packet_def_path = config_dir / packet_def_path;
    }

    // Load packet definitions
    std::ifstream packet_file(packet_def_path.string());
    if (!packet_file) {
        spdlog::error("Could not open packet definitions file: {}", packet_def_path.string());
        return 1;
    }
    
    PacketDb packet_db;
    try {
        std::string yaml_content((std::istreambuf_iterator<char>(packet_file)),
                               std::istreambuf_iterator<char>());
        packet_db = packetdb_from_yaml(yaml_content);
        spdlog::info("Loaded {} packet definitions", packet_db.size());
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse packet definitions: {}", e.what());
        return 1;
    }

    spdlog::info("Starting TCP <-> MQTT Bridge");

    ServerManager server(config, packet_db);
    server.run();

    return 0;
}