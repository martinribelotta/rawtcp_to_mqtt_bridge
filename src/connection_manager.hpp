#pragma once
#include "tcp_context.hpp"
#include "slip.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <string>

class ConnectionManager {
public:
    explicit ConnectionManager(boost::asio::ip::tcp::socket& socket);

    void handlePacket(std::span<const uint8_t> packet);
    void handleData(std::span<const uint8_t> data);
    void reset();

    const std::string& address() const { return address_; }

private:
    void sendResponse(const std::vector<uint8_t>& response);

    boost::asio::ip::tcp::socket& socket_;
    std::string address_;
    slip::Decoder decoder_;
};
