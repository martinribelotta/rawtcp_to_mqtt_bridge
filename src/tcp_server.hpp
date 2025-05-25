#ifndef RAWTCP_TO_MQTT_BRIDGE_TCP_SERVER_HPP
#define RAWTCP_TO_MQTT_BRIDGE_TCP_SERVER_HPP

#include "tcp_session.hpp"
#include "tcp_events.hpp"
#include <boost/asio.hpp>
#include <set>

class TcpServer {
public:
    TcpServer(boost::asio::io_context& io_context, 
              const boost::asio::ip::address& addr,
              unsigned short port)
        : acceptor_(io_context,
            boost::asio::ip::tcp::endpoint(addr, port))
    {
        do_accept();
    }

    void setEvents(TcpEvents events) {
        events_ = std::move(events);
    }

    ~TcpServer() {
        for (auto& session : sessions_) {
            session->stop();
        }
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    auto session = std::make_shared<TcpSession>(std::move(socket), events_);
                    sessions_.insert(session);
                    session->start();
                    
                    // Setup completion handler to remove session
                    auto weak_session = std::weak_ptr(session);
                    boost::asio::post(socket.get_executor(), [this, weak_session]() {
                        if (auto session = weak_session.lock()) {
                            sessions_.erase(session);
                        }
                    });
                }
                do_accept();
            });
    }

    boost::asio::ip::tcp::acceptor acceptor_;
    TcpEvents events_;
    std::set<std::shared_ptr<TcpSession>> sessions_;
};

#endif // RAWTCP_TO_MQTT_BRIDGE_TCP_SERVER_HPP
