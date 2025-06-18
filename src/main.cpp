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

    try { 
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

        // Process each packet definition directory
        PacketDb packet_db;
        std::filesystem::path config_dir = std::filesystem::path(config_path).parent_path();
        
        for (const auto& path : config.packet_defs.paths) {
            std::filesystem::path full_path = path[0] == '/' ? 
                std::filesystem::path{path}: config_dir / path;
            
            if (!std::filesystem::exists(full_path)) {
                spdlog::warn("Packet definitions path does not exist: {}", full_path.string());
                continue;
            }

            try {
                for (const auto& pattern : config.packet_defs.patterns) {
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(full_path)) {
                        if (!entry.is_regular_file()) continue;
                        
                        const auto& file_path = entry.path();
                        if (!std::filesystem::path(pattern).filename().string().empty() && 
                            !file_path.filename().string().ends_with(pattern.substr(1))) {
                            continue;
                        }

                        std::ifstream packet_file(file_path);
                        if (!packet_file) {
                            spdlog::error("Could not open packet definitions file: {}", file_path.string());
                            continue;
                        }

                        std::string yaml_content((std::istreambuf_iterator<char>(packet_file)),
                                            std::istreambuf_iterator<char>());
                        auto new_packets = packetdb_from_yaml(yaml_content);
                        packet_db.insert(packet_db.end(), new_packets.begin(), new_packets.end());
                        spdlog::info("Loaded {} packet definitions from {}", 
                                    new_packets.size(), file_path.string());
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("Error processing packet definitions in {}: {}", 
                            full_path.string(), e.what());
                return 1;
            }
        }

        if (packet_db.empty()) {
            spdlog::error("No packet definitions were loaded");
            return 1;
        }
        
        spdlog::info("Loaded {} total packet definitions", packet_db.size());

        spdlog::info("Starting TCP <-> MQTT Bridge");

        ServerManager server(config, packet_db);
        server.run();
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse packet definitions: {}", e.what());
        return 1;
    }

    return 0;
}