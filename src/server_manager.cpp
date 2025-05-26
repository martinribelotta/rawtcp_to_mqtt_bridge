#include "server_manager.hpp"
#include <spdlog/spdlog.h>
#include "connection_manager.hpp" // Add this include if ConnectionManager is correct

ServerManager::ServerManager(const Configuration& config)
    : config_(config)
    , server_(io_ctx_, 
             boost::asio::ip::make_address(config.tcp.bind_address), 
             config.tcp.port)
{
    setupEventHandlers();
}

void ServerManager::setupEventHandlers() {
    TcpEvents events;
    events.onConnect = [](auto& socket, auto context) {
        auto manager = std::make_shared<ConnectionManager>(socket);
        context->set("connection_manager", manager);
        spdlog::info("New client connected from {}", manager->address());
    };

    events.onDisconnect = [](auto& socket, auto context) {
        if (auto* manager = context->template get_if<std::shared_ptr<ConnectionManager>>("connection_manager")) {
            spdlog::info("Client disconnected from {}", (*manager)->address());
        }
    };

    events.onDataReceived = [](auto& socket, auto context, std::span<const uint8_t> data) {
        if (auto* manager = context->template get_if<std::shared_ptr<ConnectionManager>>("connection_manager")) {
            (*manager)->handleData(data);
        }
    };
    
    server_.setEvents(std::move(events));
}

void ServerManager::run() {
    spdlog::info("TCP server listening on {}:{}", 
                 config_.tcp.bind_address, config_.tcp.port);
    io_ctx_.run();
}

ServerManager::~ServerManager() {
    stop();
}

void ServerManager::stop() {
    if (!stopped_) {
        stopped_ = true;
        boost::system::error_code ec;
        io_ctx_.stop();
    }
}
