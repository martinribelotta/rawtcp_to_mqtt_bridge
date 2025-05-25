#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <cpptrace/from_current.hpp>
#include <cpptrace/cpptrace.hpp>

#include "tcp_server.hpp"
#include "slip.hpp"

#include <iostream>
#include <string>

using namespace std::literals;

struct Configuration {
    struct {
        unsigned short port = 12345;
        std::string bind_address = "0.0.0.0";
    } tcp;

    struct {
        std::string broker = "tcp://localhost:1883";
        std::string client_id = "tcp_bridge";
        std::vector<std::string> topics;
    } mqtt;

    std::string log_level = "debug";  // Change default to debug

    static Configuration fromYaml(const std::string& path) {
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
            }
            if (const auto& logging = yaml["logging"]) {
                config.log_level = logging["level"].as<std::string>("info");
            }
        } catch (const YAML::Exception& e) {
            spdlog::warn("Config parse error: {}. Using defaults.", e.what());
        }
        return config;
    }

    static spdlog::level::level_enum parseLogLevel(const std::string& level) {
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
        return it != levels.end() ? it->second : spdlog::level::info;
    }
};

int main(int argc, char* argv[]) {
    // Set default log level to debug right at the start
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

    // Load configuration
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

    boost::asio::io_context io_ctx;
    auto bind_addr = boost::asio::ip::make_address(config.tcp.bind_address);

    TcpServer server(io_ctx, bind_addr, config.tcp.port);
    
    // Set up event handlers
    TcpEvents events;
    events.onConnect = [](auto& socket, auto context) {
        auto addr = socket.remote_endpoint().address().to_string();
        context->set("remote_address", addr);
        context->set("decoder", slip::Decoder{});
        
        if (auto* decoder = context->template get_if<slip::Decoder>("decoder")) {
            // Capture socket by reference, and addr by value
            decoder->setPacketHandler([&socket, addr](std::span<const uint8_t> packet) {
                spdlog::debug("Decoded packet of {} bytes from {}", packet.size(), addr);
                auto response = slip::Decoder::makeResponse(slip::ACK);
                boost::asio::async_write(socket, boost::asio::buffer(response),
                    [addr](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                        if (ec) {
                            spdlog::error("Error sending packet to {}: {}", addr, ec.message());
                        } else {
                            spdlog::debug("Sent SLIP packet {} bytes to {}", bytes_transferred, addr);
                        }
                    });
            });
        }
        spdlog::info("New client connected from {}", addr);
    };

    events.onDisconnect = [](auto& socket, auto context) {
        spdlog::info("Client disconnected from {}", context->template get<std::string>("remote_address"));
    };

    events.onDataReceived = [](auto& socket, auto context, std::span<const uint8_t> data) {
        auto addr = context->template get<std::string>("remote_address");
        spdlog::debug("Raw data {} bytes from {}", data.size(), addr);
        
        try {
            if (auto* decoder = context->template get_if<slip::Decoder>("decoder")) {
                decoder->decode(data);
            }
        } catch (const slip::SlipError& e) {
            spdlog::error("SLIP decode error from {}: {}", addr, e.what());
            if (auto* decoder = context->template get_if<slip::Decoder>("decoder")) {
                decoder->reset();
            }
        }
    };
    
    server.setEvents(events);
    spdlog::info("TCP server listening on {}:{}", config.tcp.bind_address, config.tcp.port);

    CPPTRACE_TRY {
        io_ctx.run();
    } CPPTRACE_CATCH(const std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        cpptrace::from_current_exception().print();
    }

    return 0;
}