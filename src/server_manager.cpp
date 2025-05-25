#include "server_manager.hpp"
#include <spdlog/spdlog.h>

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
        auto addr = socket.remote_endpoint().address().to_string();
        context->set("remote_address", addr);
        context->set("decoder", slip::Decoder{});
        
        if (auto* decoder = context->template get_if<slip::Decoder>("decoder")) {
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
        spdlog::info("Client disconnected from {}", 
            context->template get<std::string>("remote_address"));
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
    
    server_.setEvents(std::move(events));
}

void ServerManager::run() {
    spdlog::info("TCP server listening on {}:{}", 
                 config_.tcp.bind_address, config_.tcp.port);
    io_ctx_.run();
}
