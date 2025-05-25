#pragma once
#include "config.hpp"
#include "tcp_server.hpp"
#include "slip.hpp"
#include <boost/asio.hpp>

class ServerManager {
public:
    explicit ServerManager(const Configuration& config);
    void run();

private:
    void setupEventHandlers();

    boost::asio::io_context io_ctx_;
    TcpServer server_;
    const Configuration& config_;
};
