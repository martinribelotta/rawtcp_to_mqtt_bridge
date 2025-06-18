#ifndef RAWTCP_TO_MQTT_BRIDGE_TCP_EVENTS_HPP
#define RAWTCP_TO_MQTT_BRIDGE_TCP_EVENTS_HPP

#include <boost/asio.hpp>
#include <functional>
#include <span>
#include "tcp_context.hpp"

using DataHandler = std::function<void(boost::asio::ip::tcp::socket&, std::shared_ptr<TcpContext>, std::span<const uint8_t>)>;
using ConnectionHandler = std::function<void(boost::asio::ip::tcp::socket&, std::shared_ptr<TcpContext>)>;

struct TcpEvents {
    DataHandler onDataReceived;
    ConnectionHandler onConnect;
    ConnectionHandler onDisconnect;
};

#endif // RAWTCP_TO_MQTT_BRIDGE_TCP_EVENTS_HPP
