#ifndef RAWTCP_TO_MQTT_BRIDGE_TCP_SESSION_HPP
#define RAWTCP_TO_MQTT_BRIDGE_TCP_SESSION_HPP

#include "tcp_events.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <array>

class TcpSession : public std::enable_shared_from_this<TcpSession> {
public:
    using CloseHandler = std::function<void()>;

    TcpSession(boost::asio::ip::tcp::socket socket, TcpEvents events)
        : socket_(std::move(socket))
        , events_(events)
        , context_(std::make_shared<TcpContext>())
    {
        context_->set("remote_address", socket_.remote_endpoint().address().to_string());
        if (events_.onConnect)
            events_.onConnect(socket_, context_);
    }

    ~TcpSession() {
        if (events_.onDisconnect)
            events_.onDisconnect(socket_, context_);
    }

    void start() {
        do_read();
    }

    void setCloseHandler(CloseHandler handler) {
        closeHandler_ = std::move(handler);
    }

    void stop() {
        if (!stopped_) {
            stopped_ = true;
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);
            if (closeHandler_) {
                closeHandler_();
            }
        }
    }

private:
    void do_read() {
        auto self = shared_from_this();
        socket_.async_read_some(boost::asio::buffer(data_),
            [this, self](const auto& ec, auto length) {
                if (!ec) {
                    if (events_.onDataReceived) {
                        events_.onDataReceived(socket_, context_, std::span<const uint8_t>(data_.data(), length));
                    }
                    do_read();
                } else if (ec != boost::asio::error::operation_aborted) {
                    stop();
                }
            });
    }

    boost::asio::ip::tcp::socket socket_;
    std::array<uint8_t, 1024> data_;
    TcpEvents events_;
    std::shared_ptr<TcpContext> context_;
    bool stopped_{false};
    CloseHandler closeHandler_;
};

#endif // RAWTCP_TO_MQTT_BRIDGE_TCP_SESSION_HPP
