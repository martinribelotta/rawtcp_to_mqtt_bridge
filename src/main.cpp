#include "config.hpp"
#include "server_manager.hpp"
#include <boost/program_options.hpp>
#include <cpptrace/cpptrace.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

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

    auto config = Configuration::fromYaml(vm["config"].as<std::string>());

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

    spdlog::info("Starting TCP <-> MQTT Bridge");

    ServerManager server(config);
    server.run();

    return 0;
}